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
    mage_flag_dirty(0, 127);
    mage_flag_dirty(30, 127);
    mage_flag_dirty(12, 84);
    mage_flag_dirty(7, 12);
    mage_flag_dirty(22, 66);
    mage_set_block(0, 0, 0xEB);
    mage_set_block(0, 1, 0xD7);

    mage_set_block(0, 2, 0xEB); mage_set_block_attr(0, 2, MAGE_ATTR_BRIGHTER);
    mage_set_block(0, 3, 0xD7); mage_set_block_attr(0, 3, MAGE_ATTR_BRIGHTER);

    mage_set_block(30, 126, 0xEB);
    mage_set_block(30, 127, 0xD7);
    mage_render();

	while (!kbhit());

 	mage_done();

	return 0;
}