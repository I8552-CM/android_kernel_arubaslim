#ifndef __BQ27425_FUELGUAGE_H__
#define __BQ27425_FUELGUAGE_H__

extern int is_attached;
unsigned int bq27425_get_vcell(void);
unsigned int bq27425_get_raw_vcell(void);
unsigned int bq27425_get_soc(void);
int bq27425_get_temperature(void);
unsigned int bq27425_get_current(void);
int bq27425_reset_soc(void);
int bq27425_soft_reset_soc(void);
unsigned int bq27425_get_flag(void);
unsigned int bq27425_get_fuelalert_code(void);

void bq27425_set_charger_detached(void);
unsigned int bq27425_get_recalc_soc(void);
unsigned int bq27425_get_calc_soc(void);

#endif
