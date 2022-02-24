//#include <partner.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <unistd.h>
//#include "../common/avdc.h"
//#include "../common/gdp.h"
//#include "../common/utils.h"
#include "mage.h"
#include "mgdemo.h"

int main() {
    mage_init();
    mage_flag_dirty(0, 0);
    mage_flag_dirty(10, 0);
    mage_flag_dirty(10, 10);
    mage_flag_dirty(30, 0);
    mage_flag_dirty(0, 99);
    mage_flag_dirty(39, 99);
    mage_flag_dirty(12, 84);
    mage_flag_dirty(7, 12);
    mage_flag_dirty(22, 66);
    mage_set(0, 0, 0x3a);
    mage_set(0, 1, 0x30);
    mage_set(0, 2, 0x35);
    mage_set(1, 0, 0x2b);
    mage_set(1, 1, 0x03);
    mage_set(1, 2, 0x17);

    mage_set(38, 97, 0x3a | MAGE_ATTR_DIM);
    mage_set(38, 98, 0x30 | MAGE_ATTR_DIM);
    mage_set(38, 99, 0x35 | MAGE_ATTR_DIM);
    mage_set(39, 97, 0x2b | MAGE_ATTR_DIM);
    mage_set(39, 98, 0x03 | MAGE_ATTR_DIM);
    mage_set(39, 99, 0x17 | MAGE_ATTR_DIM);


    mage_set(0, 97, 0x3a | MAGE_ATTR_BRIGHT);
    mage_set(0, 98, 0x30 | MAGE_ATTR_BRIGHT);
    mage_set(0, 99, 0x35 | MAGE_ATTR_BRIGHT);
    mage_set(1, 97, 0x2b | MAGE_ATTR_BRIGHT);
    mage_set(1, 98, 0x03 | MAGE_ATTR_BRIGHT);
    mage_set(1, 99, 0x17 | MAGE_ATTR_BRIGHT);



    //mage_set(0, 2, 0xEB);
    //mage_set(0, 3, 0xD7);

    //mage_set(30, 126, 0xEB);
    //mage_set(30, 127, 0xD7);
    mage_render();

	while (!kbhit());

 	mage_done();

	return 0;
}