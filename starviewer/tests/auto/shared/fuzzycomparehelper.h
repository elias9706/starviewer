#ifndef FUZZYCOMPAREHELPER_H
#define FUZZYCOMPAREHELPER_H

namespace testing {

/// Classe que contindrÓ les funcions de comparaciˇ fuzzy que necessitem
class FuzzyCompareHelper {
public:
    /// Fa una comparaciˇ fuzzy de dos valors double. Tenim un tercer parÓmetre opcional epsilon que ens 
    /// permetrÓ fixar la precisiˇ que volem en la comparaciˇ.
    static bool fuzzyCompare(const double &v1, const double &v2, const double &epsilon = 0.000000001);
};

}

#endif // FUZZYCOMPAREHELPER_H
