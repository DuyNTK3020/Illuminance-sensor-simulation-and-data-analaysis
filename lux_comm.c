#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void log_error(int error_code, char *description, int line_number)
{

    FILE *task3_log = fopen("task3.log", "a"); // Append to log file

    // Write the error message in the log file
    if (error_code == 4)
    {
        fprintf(task3_log, "Error 04: %s at line %d\n", description, line_number);
    }
    else
    {
        fprintf(task3_log, "Error %02d: %s\n", error_code, description);
    }

    // Close the log file
    fclose(task3_log);
}

void check_wrong_intput(const char *datafile)
{
    FILE *input_file = fopen(datafile, "r");
    char line[100]; // Assuming max line length is 100
    int line_number = 0;
    while (fgets(line, 100, input_file) != NULL)
    {
        if (line_number == 0)
        {
            // Check header format
            char *id_str = strtok(line, ",");
            char *time_str = strtok(NULL, ",");
            char *value_str = strtok(NULL, ",");
            if (id_str == NULL || strcmp(id_str, "id") != 0 ||
                time_str == NULL || strcmp(time_str, "time") != 0 ||
                value_str == NULL || strcmp(value_str, "value\n") != 0)
            {
                log_error(2, "invalid csv file format", 0);
            }
        }
        else
        {
            // Check data format
            char *id_str = strtok(line, ",");
            char *time_str = strtok(NULL, ",");
            char *value_str = strtok(NULL, ",");
            if (id_str == NULL || atoi(id_str) < 0 ||
                time_str == NULL || strstr(time_str, ":") == NULL ||
                value_str == NULL || atof(value_str) < 0.0)
            {
                log_error(4, "Error 04: data is missing", line_number);
            }
        }
        line_number++;
    }
    fclose(input_file);
}

unsigned int convertTimestamp(const char *timestamp)
{
    unsigned int year, month, day, hour, minute, second;
    sscanf(timestamp, "%u:%u:%u %u:%u:%u", &year, &month, &day, &hour, &minute, &second);

    // Convert the timestamp to Unix timestamp format
    struct tm tm;
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;
    tm.tm_isdst = -1;

    time_t unixTimestamp = mktime(&tm);
    // Convert the Unix timestamp to big-endian byte order
    unsigned int bigEndianTimestamp =
        ((unixTimestamp & 0xff000000) >> 24) |
        ((unixTimestamp & 0x00ff0000) >> 8) |
        ((unixTimestamp & 0x0000ff00) << 8) |
        ((unixTimestamp & 0x000000ff) << 24);

    return bigEndianTimestamp;
}

unsigned int luxToFloat(float lux)
{
    unsigned int sign, exponent, fraction;

    // Convert the Lux concentration to binary format
    unsigned int luxBits = *(unsigned int *)&lux;

    // Extract the sign, exponent, and fraction fields
    sign = (luxBits >> 31) & 0x1;
    exponent = ((luxBits >> 23) & 0xff) - 127;
    fraction = luxBits & 0x7fffff;

    // Combine the sign, exponent, and fraction fields into a 32-bit binary value
    unsigned int floatBits = (sign << 31) | ((exponent + 127) << 23) | fraction;

    // Convert the 32-bit binary value to big-endian byte order manually
    unsigned int bigEndianFloatBits =
        ((floatBits & 0xff000000) >> 24) |
        ((floatBits & 0x00ff0000) >> 8) |
        ((floatBits & 0x0000ff00) << 8) |
        ((floatBits & 0x000000ff) << 24);

    return bigEndianFloatBits;
}

void appendHexPacket(FILE *file, const unsigned char *packet, size_t length)
{
    for (size_t i = 0; i < length - 1; i++)
    {
        fprintf(file, "%02X ", packet[i]);
    }
    fprintf(file, "\n");
}

unsigned char calculateChecksum(const unsigned char *packet, size_t length)
{

    unsigned int sum = 0;
    // Sum the bytes of the data packet
    for (unsigned int i = 0; i < length; i++)
    {

        sum += packet[i];
    }

    // Take the two's complement of the sum
    unsigned char checksum = (~sum + 1) & 0xFF;

    return checksum;
}

void convertToHexPacket(const char *dataFilename, const char *hexFilename)
{
    FILE *dataFile = fopen(dataFilename, "r");

    FILE *hexFile = fopen(hexFilename, "w");

    // read and check in the header row of the input file
    char header[100];
    fgets(header, 100, dataFile);
    char line[256];

    while (fgets(line, sizeof(line), dataFile))
    {
        // Skip empty lines and lines starting with a comment character
        if (line[0] == '\n' || line[0] == '#')
        {
            continue;
        }

        // Extract the values from the CSV line
        unsigned int id;
        char timestamp[20];
        unsigned int location;
        double lux;
        char condition[20];

        // sscanf(line, "%u,%19[^,],%lf,%u,%19s", &id, timestamp, &value, &aqi, pollution);
        sscanf(line, "%u,%19[^,],%u,%lf,%19s", &id, timestamp, &location, &lux, condition);

        // Convert the values to the corresponding packet format
        unsigned char packet[20];
        packet[0] = 0xA0; // Start byte

        packet[1] = 0x0E; // Packet length (14 bytes)

        packet[2] = (unsigned char)id; // ID

        unsigned int timestampValue = convertTimestamp(timestamp);
        memcpy(&packet[3], &timestampValue, sizeof(timestampValue)); // Time

        packet[7] = (unsigned char)location;

        unsigned int luxValue = luxToFloat((float)lux);
        memcpy(&packet[8], &luxValue, sizeof(luxValue)); // Lux concentration

        unsigned int checksum = calculateChecksum(packet, sizeof(packet) - 2); // Exclude the start byte and checksum itself
        memcpy(&packet[12], &checksum, sizeof(checksum));                      // Checksum
        packet[13] = 0xA9;                                                     // Stop byte

        // Write the packet as a hex string to the output file
        appendHexPacket(hexFile, packet, sizeof(packet));
    }

    fclose(dataFile);
    fclose(hexFile);
}

void checkCondition(int min, int max, float lux, FILE *csvFile)
{
    if (lux < min)
    {
        fprintf(csvFile, "dark\n");
    }
    else if ((min <= lux) && (lux <= max))
    {
        fprintf(csvFile, "good\n");
    }
    else
    {
        fprintf(csvFile, "bright\n");
    }
}

void convertToCsvPacket(const char *hexFilename, const char *csvFilename)
{
    FILE *hexFile = fopen(hexFilename, "r");

    FILE *csvFile = fopen(csvFilename, "w");

    fprintf(csvFile, "id,time,location,value,condition\n");
    char line[256];

    while (fgets(line, sizeof(line), hexFile))
    {
        unsigned char packet[15];

        sscanf(line, "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X", &packet[0], &packet[1], &packet[2], &packet[3], &packet[4], &packet[5], &packet[6], &packet[7],
               &packet[8], &packet[9], &packet[10], &packet[11], &packet[12], &packet[13]);

        unsigned int id;
        char timestamp[20];
        unsigned int location;
        float lux;
        char condition[20];

        id = (int)packet[2];
        fprintf(csvFile, "%d,", id);

        unsigned int timestampValue; // Giá trị thời gian từ mảng packet

        // Sao chép giá trị thời gian từ mảng packet vào biến timestampValue
        memcpy(&timestampValue, &packet[3], sizeof(timestampValue));

        // Chuyển đổi timestamp từ big-endian byte order sang dạng Unix timestamp
        unsigned int unixTimestamp =
            ((timestampValue & 0xff000000) >> 24) |
            ((timestampValue & 0x00ff0000) >> 8) |
            ((timestampValue & 0x0000ff00) << 8) |
            ((timestampValue & 0x000000ff) << 24);

        // Chuyển đổi Unix timestamp thành struct tm
        time_t timeValue = unixTimestamp;
        struct tm *tm = localtime(&timeValue);

        // Lấy các thành phần thời gian từ struct tm
        int year = tm->tm_year + 1900;
        int month = tm->tm_mon + 1;
        int day = tm->tm_mday;
        int hour = tm->tm_hour;
        int minute = tm->tm_min;
        int second = tm->tm_sec;

        // Sử dụng các giá trị thời gian để tạo chuỗi timestamp
        fprintf(csvFile, "%04d:%02d:%02d %02d:%02d:%02d,", year, month, day, hour, minute, second);

        location = (unsigned int)packet[7];
        fprintf(csvFile, "%d,", location);

        unsigned char bytes[4]; // Mảng chứa 4 byte từ packet[8] đến packet[11]

        for (int i = 0; i < 4; ++i)
        {
            bytes[i] = packet[8 + i];
        }

        unsigned int combinedFloatBits = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];

        // Chuyển số float thành dạng float
        lux = *(float *)&combinedFloatBits;

        fprintf(csvFile, "%.2f,", lux);

        switch (location)
        {
        case 0:
            fprintf(csvFile, "N/A\n");
            break;
        case 1:
            checkCondition(20, 50, lux, csvFile);
            break;
        case 2:
            checkCondition(50, 100, lux, csvFile);
            break;
        case 3:
            checkCondition(100, 200, lux, csvFile);
            break;
        case 4:
            checkCondition(100, 150, lux, csvFile);
            break;
        case 5:
            checkCondition(150, 250, lux, csvFile);
            break;
        case 6:
            checkCondition(200, 400, lux, csvFile);
            break;
        case 7:
            checkCondition(250, 350, lux, csvFile);
            break;
        case 8:
            checkCondition(300, 500, lux, csvFile);
            break;
        case 9:
            checkCondition(500, 700, lux, csvFile);
            break;
        case 10:
            checkCondition(750, 850, lux, csvFile);
            break;
        case 11:
            checkCondition(1500, 2000, lux, csvFile);
            break;
        case 12:
            checkCondition(2000, 5000, lux, csvFile);
            break;
        case 13:
            checkCondition(5000, 10000, lux, csvFile);
            break;
        case 14:
            checkCondition(10000, 20000, lux, csvFile);
            break;
        }
    }

    fclose(hexFile);
    fclose(csvFile);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        log_error(1, "Invalid command line. Correct usage: C:\\dust_summary  [data_filename.csv] [hex_filename.dat]", 0);
        printf("Invalid number of arguments. Usage: dust_convert [data_filename.csv] [hex_filename.dat]\n");
        return 1;
    }
    // Check input file
    FILE *dataFile = fopen(argv[1], "r");
    if (dataFile == NULL)
    {
        log_error(1, " could not open datafile ", 0);
        printf("Error: could not open datafile: %s\n", argv[1]);
        return 1;
    }
    fclose(dataFile);
    // check hex file
    FILE *hexFile = fopen(argv[2], "r");
    if (dataFile == NULL)
    {
        log_error(5, " cannot override hex_filename.dat ", 0);
        printf("Error: cannot override  %s\n", argv[2]);
        return 1;
    }
    fclose(hexFile);

    printf("Conversion completed successfully.\n");

    if (strstr(argv[1], ".csv") != NULL)
    {
        const char *dataFilename = argv[1];
        const char *hexFilename = argv[2];

        check_wrong_intput(dataFilename);

        convertToHexPacket(dataFilename, hexFilename);
    }
    else if (strstr(argv[1], ".dat") != NULL)
    {
        const char *dataFilename = argv[1];
        const char *csvFilename = argv[2];

        check_wrong_intput(dataFilename);

        convertToCsvPacket(dataFilename, csvFilename);
    }
    return 0;
}