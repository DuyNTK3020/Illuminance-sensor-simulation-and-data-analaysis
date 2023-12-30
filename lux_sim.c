#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_NUM_SENSORS 1
#define DEFAULT_ST 60
#define DEFAULT_SI 24

void logError(int errorCode, const char *description)
{
    FILE *logPointer;
    logPointer = fopen("task1.log", "a");
    if (logPointer == NULL)
    {
        printf("Unable to open log file\n");
        exit(1);
    }
    fprintf(logPointer, "Error %02d: %s\n", errorCode, description);
    fclose(logPointer);
}

void workWithCommandLine(int argc, char *argv[], int *numSensors, int *samplingTime, int *interval)
{
    int i = 1;
    for (i = 1; i < argc; i += 2)
    {
        if (strcmp(argv[i], "-n") == 0)
        {
            *numSensors = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-st") == 0)
        {
            *samplingTime = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-si") == 0)
        {
            *interval = atoi(argv[i + 1]);
        }
        else
        {
            logError(1, "invalid command");
        }
    }
}

void workWithFile(int numSensors, int samplingTime, int interval)
{
    FILE *fPointer;
    fPointer = fopen("lux_sensor.csv", "a");
    if (fPointer == NULL)
    {
        logError(3, "file access denied");
        printf("Error in opening file");
        exit(1);
    }
    fclose(fPointer);

    fPointer = fopen("lux_sensor.csv", "w");
    if (fPointer == NULL)
    {
        logError(4, "Error in opening file lux_sensor.csv");
        printf("Error in opening file");
        exit(1);
    }
    fprintf(fPointer, "id,time,value\n");
    time_t now = time(NULL);
    time_t startingTime = now - (3600 * (interval));
    srand(startingTime); // mỗi giá trị này thì rand khác nhau
    for (int i = 0; i <= interval * 3600; i += samplingTime)
    {
        for (int j = 1; j <= numSensors; j++)
        {
            struct tm *current_time = localtime(&startingTime);
            double random, max = 40000, min = 0.1;
            random = ((double)rand() / (double)RAND_MAX) * (max - min) + min;

            fprintf(fPointer, "%d,%04d:%02d:%02d %02d:%02d:%02d,%.2lf\n", j, current_time->tm_year + 1900, current_time->tm_mon + 1, current_time->tm_mday, current_time->tm_hour, current_time->tm_min, current_time->tm_sec, random);
        }
        startingTime = startingTime + samplingTime;
    }
    fclose(fPointer);

    printf("==============================================\n");
    printf("Data is successfully created in lux_sensor.csv\n");
    printf("==============================================\n");
}

int main(int argc, char *argv[])
{
    int numSensors = DEFAULT_NUM_SENSORS;
    int samplingTime = DEFAULT_ST;
    int interval = DEFAULT_SI;
    workWithCommandLine(argc, argv, &numSensors, &samplingTime, &interval);
    if (numSensors <= 0 || samplingTime <= 0 || interval <= 0)
    {
        logError(2, "invalid argument");
        exit(1);
    }
    workWithFile(numSensors, samplingTime, interval);
    return 0;
}
