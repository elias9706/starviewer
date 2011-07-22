#ifndef PACSDEVICETESTHELPER_H
#define PACSDEVICETESTHELPER_H

#include <QString>
#include "pacsdevice.h"

namespace udg
{
/// Classe que retorna PACSDevice per utilitzar per testing
class PACSDeviceTestHelper {

public:

    /// Retorna un PacsDevice amb els camps mínims obligatoris. A AETitle, ID, Address i Institutiom se li assigna el valor de ID
    static PacsDevice createPACSDeviceByID(QString ID);
};
}

#endif // PACSDEVICETESTHELPER_H
