#ifndef zummer_h
#define zummer_h

extern const int alarm[];
extern const int battery_low[];
extern const int button[];
extern const int alarm_and_battery_low[];
extern const int bip_1000[];
extern const int bip_2000[];

void zummerInit();
void zummerStart();
void zummerStop();
void zummerRun(const int *pattern);
bool zummerIsBusy();

#endif