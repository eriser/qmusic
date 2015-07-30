#include <QDebug>
#include <QPainter>
#include <QMimeData>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneDragDropEvent>
#include <QKeyEvent>
#include "Application.h"
#include "SerializationContext.h"
#include "AudioUnitsManager.h"
#include "AudioUnitPlugin.h"
#include "AudioUnit.h"
#include "SignalChain.h"
#include "SignalChainItem.h"
#include "SignalChainPortItem.h"
#include "SignalChainConnectionItem.h"
#include "SignalChainAudioUnitItem.h"
#include "SignalChainScene.h"

const QSizeF cGridSize(8, 8);

const QString SignalChainScene::UID("SignalChainScene");

SignalChainScene::SignalChainScene(QObject *pParent)
    : QGraphicsScene(pParent)
{
    m_pSignalChain = new SignalChain();
    m_pDraggedAudioUnitPlugin = nullptr;
    m_pConnectionItem = nullptr;

    connect(this, SIGNAL(selectionChanged()), this, SLOT(onSelectionChanged()));
}

SignalChainScene::~SignalChainScene()
{
    m_pSignalChain->stop();
    delete m_pSignalChain;
}

SignalChainItem* SignalChainScene::signalChainItemAtPos(const QPointF &pos) const
{
    QList<QGraphicsItem*> graphicsItems = items(QRectF(pos - QPointF(1, 1), QSizeF(3, 3)));
    foreach (QGraphicsItem *pItem, graphicsItems) {
        if (pItem->type() >= SignalChainItem::Type_First) {
            return dynamic_cast<SignalChainItem*>(pItem);
        }
    }
    return nullptr;
}

void SignalChainScene::selectAll()
{
    foreach (QGraphicsItem *pItem, items(Qt::DescendingOrder)) {
        pItem->setSelected(true);
    }
}

void SignalChainScene::serialize(QVariantMap &data, SerializationContext *pContext) const
{
    Q_ASSERT(pContext != nullptr);

    // Serialize audio units
    data["signalChain"] = pContext->serialize(m_pSignalChain);

    // Serialize signal chain items
    QVariantList list;
    foreach (QGraphicsItem *pItem, items()) {
        if (pItem->type() >= SignalChainItem::Type_First) {
            SignalChainItem *pSignalChainItem = dynamic_cast<SignalChainItem*>(pItem);
            Q_ASSERT(pSignalChainItem != nullptr);
            QVariant handle = pContext->serialize(pSignalChainItem);
            list.append(handle);
        }
    }

    data["signalChainItems"] = list;
}

void SignalChainScene::deserialize(const QVariantMap &data, SerializationContext *pContext)
{
    Q_ASSERT(pContext != nullptr);

    // Remove all existing items
    deleteAll();

    // Deserialize audio units
    delete m_pSignalChain;
    m_pSignalChain = pContext->deserialize<SignalChain>(data["signalChain"]);

    // Deserialize signal chain items
    QVariantList list = data["signalChainItems"].toList();
    foreach (const QVariant &handle, list) {
        SignalChainItem *pItem = pContext->deserialize<SignalChainItem>(handle);
        Q_ASSERT(pItem != nullptr);
        addItem(pItem);
    }
}

void SignalChainScene::deleteSelected()
{
    QSet<SignalChainConnectionItem*> connectionsToDelete;
    QSet<SignalChainAudioUnitItem*> audioUnitsToDelete;

    foreach (QGraphicsItem *pItem, selectedItems()) {
        if (pItem->type() == SignalChainItem::Type_AudioUnit) {
            SignalChainAudioUnitItem *pAuItem = dynamic_cast<SignalChainAudioUnitItem*>(pItem);
            audioUnitsToDelete.insert(pAuItem);
            foreach (SignalChainConnectionItem *pConnItem, pAuItem->connectionItems()) {
                connectionsToDelete.insert(pConnItem);
            }
        } else if (pItem->type() == SignalChainItem::Type_Connection) {
            SignalChainConnectionItem *pConnectionItem = dynamic_cast<SignalChainConnectionItem*>(pItem);
            connectionsToDelete.insert(pConnectionItem);
        }
    }

    foreach (SignalChainConnectionItem *pItem, connectionsToDelete) {
        delete pItem;
    }

    foreach (SignalChainAudioUnitItem *pItem, audioUnitsToDelete) {
        delete pItem;
    }
}

void SignalChainScene::deleteAll()
{
    clear();
}

void SignalChainScene::mousePressEvent(QGraphicsSceneMouseEvent *pEvent)
{
    if (pEvent->button() == Qt::LeftButton) {
        SignalChainItem *pItem = signalChainItemAtPos(pEvent->scenePos());
        if (pItem != nullptr) {
            if (pItem->type() == SignalChainItem::Type_InputPort ||
                    pItem->type() == SignalChainItem::Type_OutputPort) {
                SignalChainPortItem *pPortItem = dynamic_cast<SignalChainPortItem*>(pItem);
                Q_ASSERT(pPortItem != nullptr);
                if (pPortItem->isOutput() || (pPortItem->isInput() && !pPortItem->hasConnections())) {
                //if (!pPortItem->hasConnections()) {
                    m_pConnectionItem = new SignalChainConnectionItem();
                    addItem(m_pConnectionItem);
                    m_pConnectionItem->setPort(pPortItem);
                    m_pConnectionItem->updatePath();
                    emit beginConnection();
                    return;
                }
            }
        }
    }

    QGraphicsScene::mousePressEvent(pEvent);
}

void SignalChainScene::mouseMoveEvent(QGraphicsSceneMouseEvent *pEvent)
{
    m_mousePos = pEvent->scenePos();
    if (m_pConnectionItem != nullptr) {
        if (m_pConnectionItem->inputPortItem() != nullptr) {
            m_pConnectionItem->setSourcePoint(m_mousePos);
        } else if (m_pConnectionItem->outputPortItem() != nullptr) {
            m_pConnectionItem->setTargetPoint(m_mousePos);
        }
        m_pConnectionItem->updatePath();
        return;
    }

    QGraphicsScene::mouseMoveEvent(pEvent);
}

void SignalChainScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *pEvent)
{
    if (pEvent->button() == Qt::LeftButton) {
        if (m_pConnectionItem != nullptr) {
            SignalChainItem *pItem = signalChainItemAtPos(pEvent->scenePos());
            if (pItem != nullptr) {
                if (pItem->type() == SignalChainItem::Type_InputPort) {
                    SignalChainInputPortItem *pInputItem = dynamic_cast<SignalChainInputPortItem*>(pItem);
                    if (!pInputItem->hasConnections() &&
                            m_pConnectionItem->inputPortItem() == nullptr &&
                            m_pConnectionItem->outputPortItem()->parentItem() != pInputItem->parentItem()) {
                        establishConnection(pInputItem);
                        return;
                    }
                } else if (pItem->type() == SignalChainItem::Type_OutputPort) {
                    SignalChainOutputPortItem *pOutputItem = dynamic_cast<SignalChainOutputPortItem*>(pItem);
                    if (m_pConnectionItem->outputPortItem() == nullptr &&
                            m_pConnectionItem->inputPortItem()->parentItem() != pOutputItem->parentItem()) {
                        establishConnection(pOutputItem);
                        return;
                    }
                }
            }

            delete m_pConnectionItem;
            m_pConnectionItem = nullptr;
            emit endConnection();
            return;
        }
    }

    QGraphicsScene::mouseReleaseEvent(pEvent);
}

void SignalChainScene::dragEnterEvent(QGraphicsSceneDragDropEvent *pEvent)
{
    bool acceptDrag = false;
    const QMimeData *pMimeData = pEvent->mimeData();
    if (pMimeData->formats().contains(AudioUnitPlugin::MimeDataFormat)) {
        QString uid = QString::fromUtf8(pMimeData->data(AudioUnitPlugin::MimeDataFormat));
        m_pDraggedAudioUnitPlugin = Application::instance()->audioUnitsManager()->audioUnitPluginByUid(uid);
        acceptDrag = m_pDraggedAudioUnitPlugin != nullptr;
    }

    pEvent->setAccepted(acceptDrag);
}

void SignalChainScene::dragMoveEvent(QGraphicsSceneDragDropEvent *pEvent)
{
    bool accept = (m_pDraggedAudioUnitPlugin != nullptr) && (m_pSignalChain != nullptr);
    pEvent->setAccepted(accept);
}

void SignalChainScene::dropEvent(QGraphicsSceneDragDropEvent *pEvent)
{
    AudioUnit *pAudioUnit = nullptr;

    if (m_pDraggedAudioUnitPlugin != nullptr && m_pSignalChain != nullptr) {
        pAudioUnit = m_pDraggedAudioUnitPlugin->createInstance();
        m_pDraggedAudioUnitPlugin = nullptr;
    }

    if (pAudioUnit != nullptr) {
        Q_ASSERT(m_pSignalChain != nullptr);
        m_pSignalChain->addAudioUnit(pAudioUnit);
        SignalChainAudioUnitItem *pItem = new SignalChainAudioUnitItem(pAudioUnit);
        QPointF pos = pEvent->scenePos();
        pItem->setPos(pos);
        addItem(pItem);
        pEvent->setAccepted(true);
    } else {
        pEvent->setAccepted(false);
    }
}

void SignalChainScene::keyPressEvent(QKeyEvent *pEvent)
{
    if (pEvent->key() == Qt::Key_Delete) {
        deleteSelected();
    } else if (pEvent->matches(QKeySequence::SelectAll)) {
        selectAll();
    } else if (pEvent->matches(QKeySequence::Copy)) {
        //copyToClipboard();
    } else if (pEvent->matches(QKeySequence::Paste)) {
        //pasteFromClipboard();
    } else if (pEvent->matches(QKeySequence::Cut)) {
        //copyToClipboard();
        //deleteSelected();
    }
    QGraphicsScene::keyPressEvent(pEvent);

}

void SignalChainScene::drawBackground(QPainter *pPainter, const QRectF &rect)
{
    QPainter::RenderHints hints = pPainter->renderHints();
    pPainter->setRenderHint(QPainter::Antialiasing, false);
    QPen pen;
    pen.setWidth(1);
    pPainter->setPen(QColor(240, 240, 240));

    int top = qRound(rect.top() / cGridSize.height()) * cGridSize.height();
    int left = qRound(rect.left() / cGridSize.width()) * cGridSize.width();
    int bottom = qRound((rect.bottom() + cGridSize.height()) / cGridSize.height()) * cGridSize.height();
    int right = qRound((rect.right() + cGridSize.width()) / cGridSize.width()) * cGridSize.width();

    int sx = cGridSize.width();
    int sy = cGridSize.height();

    for (int y = top; y < bottom; y += sy) {
        pPainter->drawLine(left, y, right, y);
    }

    for (int x = left; x < right; x += sx) {
        pPainter->drawLine(x, top, x, bottom);
    }

    pPainter->setRenderHints(hints);
}

void SignalChainScene::onSelectionChanged()
{
    QList<QGraphicsItem*> items = selectedItems();

    if (items.count() == 1) {
        QGraphicsItem *pItem = items.at(0);
        if (pItem->type() == SignalChainItem::Type_AudioUnit) {
            SignalChainAudioUnitItem *pAuItem = dynamic_cast<SignalChainAudioUnitItem*>(pItem);
            Q_ASSERT(pAuItem != nullptr);
            emit audioUnitSelected(pAuItem->audioUnit());
            return;
        }
    }

    // No audio unit is selected
    emit audioUnitSelected(nullptr);
}

void SignalChainScene::establishConnection(SignalChainPortItem *pFinalPort)
{
    Q_ASSERT(m_pConnectionItem != nullptr);
    Q_ASSERT(pFinalPort != nullptr);

    m_pConnectionItem->setPort(pFinalPort);
    m_pConnectionItem->updatePath();

    // Establish connection at signal chain
    InputPort *pInputPort = m_pConnectionItem->inputPortItem()->inputPort();
    OutputPort *pOutputPort = m_pConnectionItem->outputPortItem()->outputPort();
    Q_ASSERT(pInputPort != nullptr);
    Q_ASSERT(pOutputPort != nullptr);
    pInputPort->connect(pOutputPort);

    m_pConnectionItem = nullptr;
    emit endConnection();
}