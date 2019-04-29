 #define _GNU_SOURCE
#include <stdlib.h>     //exit()
#include <signal.h>     //signal()
#include <time.h>
#include "ADS1256.h"
#include "stdio.h"
#include <sys/time.h>
#include <string.h>
#include <math.h>
#include <stdint.h>     /* intmax_t */
#include <inttypes.h>   /* strtoimax, PRIdMAX, SCNdMAX */
#include <unistd.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>


void  Handler(int signo)
{
    //System Exit
    printf("\r\nEND                  \r\n");
    DEV_ModuleExit();

    exit(0);
}
int main(void)
{
    int32_t  ADC[3];
    double ADC_V[3];
    printf("Program Started\r\n");

    double _conversionFactor = 1.0;
    double _pga = 64;
    DEV_ModuleInit();
    DEV_Digital_Write(DEV_CS_PIN, 1);

    // Exception handling:ctrl + c
    signal(SIGINT, Handler);

    if(ADS1256_init() == 1){
        printf("InitFail\r\nEND\r\n");
        DEV_ModuleExit();
        exit(0);
    }
    
    DEV_Digital_Write(DEV_CS_PIN, 0);

    FILE *pFile;

    time_t current_time;
    struct tm * time_info;
    char timeString[20];  // space for "HH:MM:SS\0"
    int changed = 0;


    time(&current_time);
    time_info = localtime(&current_time);
    strftime(timeString, sizeof(timeString), "%Y%m%d_%H%M%S.txt", time_info);
    pFile = fopen( timeString,"w" );
   
    
    if( NULL == pFile ){
        printf( "open failure" );
        return 1;
    }

    int     fd[2];
    pid_t   childpid;
    char    readbuffer[1024*128];

    pipe(fd);

    if((childpid = fork()) == -1)
    {
            perror("fork");
            exit(1);
    }

    if(childpid == 0)
    {
            /* Child process closes up input side of pipe */
            /* For Reading the data from ADS1256 */
            close(fd[0]);

            struct timespec spec_last;
            clock_gettime(CLOCK_REALTIME, &spec_last);


            struct timespec sleep_spec;
            sleep_spec.tv_nsec = 1000000000 - spec_last.tv_nsec; // start from second
            nanosleep(&sleep_spec, NULL);


            clock_gettime(CLOCK_REALTIME, &spec_last);
            while(1){
                    // Reading three channel according to Figure 19 in ADS1256 Datasheet
                    while(DEV_Digital_Read(DEV_DRDY_PIN) == 1);
                    DEV_SPI_WriteByte(CMD_WREG | REG_MUX);
                    bcm2835_delayMicroseconds(30);
                    DEV_SPI_WriteByte(0x00);
                    DEV_SPI_WriteByte((0 << 4) | 1); // Switch to Diff ch 0
                    DEV_SPI_WriteByte(CMD_SYNC);
                    bcm2835_delayMicroseconds(100);
                    DEV_SPI_WriteByte(CMD_WAKEUP);
                    DEV_SPI_WriteByte(CMD_RDATA);
                    bcm2835_delayMicroseconds(30);
                    ADC[2] = ADS1256_Read_ADC_Data_Lite();
                      if (ADC[2]  & 0x00800000) {
                        ADC[2]  |= 0xff000000;
                      }
                    ADC_V[2] =  (((double)ADC[2] / 0x7FFFFF) * ((2 * 2.5) / (float)_pga)) *  _conversionFactor;

                    while(DEV_Digital_Read(DEV_DRDY_PIN) == 1);
                    DEV_SPI_WriteByte(CMD_WREG | REG_MUX);
                    bcm2835_delayMicroseconds(30);
                    DEV_SPI_WriteByte(0x00);
                    DEV_SPI_WriteByte((2 << 4) | 3); // Switch to Diff ch 1
                    DEV_SPI_WriteByte(CMD_SYNC);
                    bcm2835_delayMicroseconds(100);
                    DEV_SPI_WriteByte(CMD_WAKEUP);
                    DEV_SPI_WriteByte(CMD_RDATA);
                    bcm2835_delayMicroseconds(30);
                    ADC[0] = ADS1256_Read_ADC_Data_Lite();
                    if (ADC[0]  & 0x00800000) {
                        ADC[0]  |= 0xff000000;
                      }
                    ADC_V[0] =  (((double)ADC[0] / 0x7FFFFF) * ((2 * 2.5) / (float)_pga)) *  _conversionFactor;

                    while(DEV_Digital_Read(DEV_DRDY_PIN) == 1);
                    DEV_SPI_WriteByte(CMD_WREG | REG_MUX);
                    bcm2835_delayMicroseconds(30);
                    DEV_SPI_WriteByte(0x00);
                    DEV_SPI_WriteByte((4 << 4) | 5); // Switch to Diff ch 2
                    DEV_SPI_WriteByte(CMD_SYNC);
                    bcm2835_delayMicroseconds(100);
                    DEV_SPI_WriteByte(CMD_WAKEUP);
                    DEV_SPI_WriteByte(CMD_RDATA);
                    bcm2835_delayMicroseconds(30);
                    ADC[1] = ADS1256_Read_ADC_Data_Lite();
                    if (ADC[1]  & 0x00800000) {
                        ADC[1]  |= 0xff000000;
                      }
                    ADC_V[1] =  (((double)ADC[1] / 0x7FFFFF) * ((2 * 2.5) / (float)_pga)) *  _conversionFactor;

                    time_t          s;  // Seconds
                    struct timespec spec;

                    clock_gettime(CLOCK_REALTIME, &spec);

                    s  = spec.tv_sec;
                    long durition;
                    durition = spec.tv_nsec - spec_last.tv_nsec + (spec.tv_sec -  spec_last.tv_sec)*1000000000UL;

                    char cBuffer[200];
                    sprintf(cBuffer, "%"PRIdMAX".%09ld,%.9lf,%.9lf,%.9lf,%09ld\n",(intmax_t)s, spec.tv_nsec,ADC_V[0],ADC_V[1],ADC_V[2],durition);
                    write(fd[1], cBuffer, (strlen(cBuffer)+1));
                    clock_gettime(CLOCK_REALTIME, &spec_last);
            }
            exit(0);
    }
    else
    {
            /* Parent process closes up output side of pipe */
            /* For writing the data to files */
            close(fd[1]);
            fcntl(fd[0],F_SETPIPE_SZ,1048576); 
            while(1){
                read(fd[0], readbuffer, sizeof(readbuffer));
                //printf("Received string: %s", readbuffer);
                fwrite(readbuffer,1,strlen(readbuffer),pFile);
                time(&current_time);
                time_info = localtime(&current_time);
                if ((time_info->tm_min % 5 == 0) & (changed == 0)) //Switch file per 5 min
                {
                    strftime(timeString, sizeof(timeString), "%Y%m%d_%H%M%S.txt", time_info);
                    fclose(pFile);
                    pFile = fopen( timeString,"w" );
                    changed = 1;
                }
                if ((time_info->tm_min % 5 == 1) & (changed == 1))
                {
                    changed = 0;
                }
            }

    }
    
    DEV_Digital_Write(DEV_CS_PIN, 1);
    return 0;
}

