#ifndef PORT_H
#define PORT_H

#include <QVariant>
#include "FrameworkApi.h"

class IAudioUnit;

class QMUSIC_FRAMEWORK_API Port
{
public:

    /// Port data flow direction.
    enum Direction {
        Direction_Input,
        Direction_Output
    };

    /**
     * @brief Construct a port.
     * @param dir Data flow direction.
     * @param name Port name.
     * @param type Data type.
     */
    Port(Direction dir = Direction_Input,
         const QString &name = QString(),
         QVariant::Type type = QVariant::Invalid);

    /// Destructor.
    virtual ~Port() {}

    Direction direction() const { return m_direction; }
    QString name() const { return m_name; }
    QVariant::Type dataType() const { return m_dataType; }

    /**
     * Returns index of this port within the set of audio unit ports.
     * @return
     */
    virtual int index() const = 0;

    void setAudioUnit(IAudioUnit *pAudioUnit) { m_pAudioUnit = pAudioUnit; }
    IAudioUnit* audioUnit() const { return m_pAudioUnit; }

    /**
     * @brief Returns value currently set on this port.
     * @return Signal data value.
     */
    virtual QVariant value() const = 0;

    /**
     * @brief Update signal chain.
     */
    virtual void update() = 0;

private:

    Direction m_direction;      ///< Data flow direction.
    QString m_name;             ///< Port name.
    QVariant::Type m_dataType;  ///< Data type.

    /// Pointer to the audio unit this port belongs to.
    IAudioUnit *m_pAudioUnit;

};

#endif // PORT_H

