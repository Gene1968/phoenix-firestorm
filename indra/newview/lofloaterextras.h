#ifndef LOLISTORM_LOFLOATEREXTRAS_H
#define LOLISTORM_LOFLOATEREXTRAS_H

#include "llfloater.h"

class LOFloaterExtras : public LLFloater
{
public:
    LOFloaterExtras(const LLSD& key);
    /*virtual*/ ~LOFloaterExtras();

    /*virtual*/ bool postBuild();

    void update_labels();
    void logged_in();
};

#endif // ShareStorm LOLISTORM_LOFLOATERSPOOF_H
