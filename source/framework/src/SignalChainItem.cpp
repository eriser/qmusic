#include "SerializationContext.h"
#include "SignalChainItem.h"

SignalChainItem::SignalChainItem(Type type, QGraphicsItem *pParent)
    : QGraphicsPathItem(pParent),
      m_type(type)
{
}
