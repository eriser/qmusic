#ifndef INPUTPORT_H
#define INPUTPORT_H

#include <QVariant>
#include "Port.h"
#include "FrameworkApi.h"

class OutputPort;

class QMUSIC_FRAMEWORK_API InputPort : public Port
{
public:

    InputPort();
    InputPort(const QString &name, QVariant::Type type);

    QVariant value() const override;
    void update() override;
    int index() const override final;

    void connect(OutputPort *pOutput);
    void disconnect();

    OutputPort* connectedOutputPort() const { return m_pConnectedOutputPort; }

private:

    /// Pointer to connected output port, if any.
    OutputPort *m_pConnectedOutputPort;
};

#endif // INPUTPORT_H

