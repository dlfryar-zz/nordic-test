#include "mbed.h"
#include "gnss.h"

#define CHECK_TALKER(s) ((buffer[3] == s[0]) && (buffer[4] == s[1]) && (buffer[5] == s[2]))

// LEDs
DigitalOut ledRed(LED1, 1);
DigitalOut ledGreen(LED2, 1);
DigitalOut ledBlue(LED3, 1);
DigitalOut ledFour(LED4, 1);

Thread spi_thread;

void thread_spi() {
    while (true) {
        ledFour = !ledFour;
        wait_ms(200);
    }
}

int main()
{
    spi_thread.start(thread_spi);

#if 1
    GnssI2C gnss(p26, p27);
#else
    GnssI2C gnss(I2C_SDA, I2C_SCL);
#endif
    int gnssReturnCode;
    int length;
    char buffer[256];

    printf("Starting up...\n");
    if (gnss.init(NC))
    {
        printf("Waiting for GNSS to receive something...\n");
        while (1)
        {
            gnssReturnCode = gnss.getMessage(buffer, sizeof(buffer));
            if (gnssReturnCode > 0)
            {
                ledGreen = 0;
                ledBlue = 1;
                ledRed = 1;
                length = LENGTH(gnssReturnCode);

                printf("NMEA: %.*s\n", length - 2, buffer);

                if ((PROTOCOL(gnssReturnCode) == GnssParser::NMEA) && (length > 6))
                {
                    // Talker is $GA=Galileo $GB=Beidou $GL=Glonass $GN=Combined $GP=GNSS
                    if ((buffer[0] == '$') || buffer[1] == 'G')
                    {
                        if (CHECK_TALKER("GLL"))
                        {
                            double latitude = 0, longitude = 0;
                            char ch;

                            if (gnss.getNmeaAngle(1, buffer, length, latitude) &&
                                gnss.getNmeaAngle(3, buffer, length, longitude) &&
                                gnss.getNmeaItem(6, buffer, length, ch) && (ch == 'A'))
                            {
                                ledBlue = 0;
                                ledRed = 0;
                                ledGreen = 0;

                                printf("\nGNSS: location is %.5f %.5f.\n\n", latitude, longitude);
                                printf("I am here: https://maps.google.com/?q=%.5f,%.5f\n\n",
                                       latitude, longitude);
                            }
                        }
                        else if (CHECK_TALKER("GGA") || CHECK_TALKER("GNS"))
                        {
                            double altitude = 0;
                            const char *timeString = NULL;

                            // Altitude
                            if (gnss.getNmeaItem(9, buffer, length, altitude))
                            {
                                printf("\nGNSS: altitude is %.1f m.\n", altitude);
                            }

                            // Time
                            timeString = gnss.findNmeaItemPos(1, buffer, buffer + length);
                            if (timeString != NULL)
                            {
                                ledBlue = 0;
                                ledRed = 1;
                                ledGreen = 1;

                                printf("\nGNSS: time is %.6s.\n\n", timeString);
                            }
                        }
                        else if (CHECK_TALKER("VTG"))
                        {
                            double speed = 0;

                            // Speed
                            if (gnss.getNmeaItem(7, buffer, length, speed))
                            {
                                printf("\nGNSS: speed is %.1f km/h.\n\n", speed);
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        printf("Unable to initialise GNSS.\n");
    }

    ledRed = 0;
    ledGreen = 1;
    ledBlue = 1;
    printf("Should never get here.\n");
    MBED_ASSERT(false);
}

// End Of File