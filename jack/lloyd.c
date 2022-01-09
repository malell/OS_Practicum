
#include "lloyd.h"


#define PATH_FILE   "Halloran.txt"
#define TIMETOW     120
#define FIL_ERROR   "Could not open file\n"

#define STATION_NAME    "Nom d'estacio: %s\n"
#define TEMP_MEAN       "\tTemperatura mitjana: %.2f\n"
#define HUM_MEAN        "\tHumitat mitjana: %.2f\n"
#define PRES_MEAN       "\tPressio atmosferica mitjana: %.2f\n"
#define PREC_MEAN       "\tPrecipitacio mitjana: %.2f\n\n"

#define MAX_BUFFER  200

Statistics **stats;
Data *datum;
int nStats, fd, isAlarm = 0;

/**
 * function that saves current data statistics to txt file
 */
void saveData(void){
    _unlink(PATH_FILE);
    fd = open(PATH_FILE, O_CREAT | O_WRONLY, 0600);
    if (fd < 0) {
        write(1, FIL_ERROR, strlen(FIL_ERROR));
    }else{
        char buff[MAX_BUFFER];
        int size;
        for(int i = 0; i < nStats; i++){
            if(!*stats[i]->station) break;
            memset(buff, '\0', MAX_BUFFER);
            size = sprintf(buff, STATION_NAME, stats[i]->station);
            write(fd, buff, size);

            memset(buff, '\0', MAX_BUFFER);
            size = sprintf(buff, TEMP_MEAN, stats[i]->temperature);
            write(fd, buff, size);

            memset(buff, '\0', MAX_BUFFER);
            size = sprintf(buff, HUM_MEAN, stats[i]->humidity);
            write(fd, buff, size);

            memset(buff, '\0', MAX_BUFFER);
            size = sprintf(buff, PRES_MEAN, stats[i]->pressure);
            write(fd, buff, size);

            memset(buff, '\0', MAX_BUFFER);
            size = sprintf(buff, PREC_MEAN, stats[i]->precipitation);
            write(fd, buff, size);
        }
        //free(buff);
        close(fd);
    }
}

/**
 * function that frees allocated memory for statistics
 */
void freeData(){
    for(int i = nStats+1; i > 0; i--){
        free(stats[i]);
    }
    free(stats[0]);
    free(stats);
}

/**
 * RSI for signals
 * @param sig Signal received
 */
void sigHandlerLloyd(int sig){
    switch(sig){
        case SIGINT:

            freeData();
            shmdt(datum);
            close(fd);
            exit(EXIT_SUCCESS);
            break;
        case SIGALRM:

            isAlarm = 1;
            saveData();
            alarm(TIMETOW);
            break;
        default:
            break;
    }
    signal(sig, sigHandlerLloyd);
}


/**
 * Initialize dynamic memory for statistics
 */
void initializeStats(void){
    stats = (Statistics**) malloc(sizeof(Statistics*));
    stats[0] = (Statistics*) malloc(sizeof(Statistics));
    memset(stats[0]->station, '\0', MAX_CHAR);
    stats[0]->lectures = 0;
    stats[0]->temperature = 0;
    stats[0]->humidity = 0;
    stats[0]->pressure = 0;
    stats[0]->precipitation = 0;
}

/**
 * Initialize Shared memory pointer
 */
void initializeSharedMemory(void){
    memid = shmget(ftok("lloyd.c",1),sizeof(Data),IPC_EXCL|0600);
    datum = shmat(memid,NULL,0);
}

/**
 * Function that checks wether a station name is found i current statistics array
 * @param name Name of station
 * @return either index in array of station or -1 if not found
 */
int statsIndex(char* name){
    for(int i = 0; i < nStats; i++){
        if(!strcmp(stats[i]->station, name))
            return i;
    }
    return -1;
}

/**
 * Process readed data from shared memory. Either operates it and reallocates dynamic memory if necessary
 * @param dades Data readed from shared memory
 */
void processData(Data dades){
    //Comprovem si el nom de l'estació es troba a Statistics
    int index = statsIndex(dades.name);
    if(index != -1){
        //Si es troba, augmentem lectures i fem nova mitjana
        ++stats[index]->lectures;
        stats[index]->temperature = (stats[index]->temperature*(stats[index]->lectures-1) + dades.temperature)/((stats[index]->lectures));
        stats[index]->humidity = (stats[index]->humidity*(stats[index]->lectures-1) + dades.humidity)/((stats[index]->lectures));
        stats[index]->pressure = (stats[index]->pressure*(stats[index]->lectures-1) + dades.pressure)/((stats[index]->lectures));
        stats[index]->precipitation = (stats[index]->precipitation*(stats[index]->lectures-1) + dades.precipitation)/((stats[index]->lectures));
    }else{
        //Si no, realloc i inicialització
        if(nStats != 0){
            stats = (Statistics**) realloc(stats, sizeof(Statistics*)*(nStats+1));
            stats[nStats] = (Statistics*) malloc(sizeof(Statistics));
            memset(stats[nStats]->station,'\0', MAX_CHAR);
            stats[nStats]->lectures = 0;
            stats[nStats]->temperature = 0.;
            stats[nStats]->humidity = 0.;
            stats[nStats]->pressure = 0.;
            stats[nStats]->precipitation = 0.;
        }

        strncpy_s(stats[nStats]->station, dades.name, MAX_CHAR);
        stats[nStats]->lectures = 1;
        stats[nStats]->temperature = (float) dades.temperature;
        stats[nStats]->humidity = (float) dades.humidity;
        stats[nStats]->pressure = (float) dades.pressure;
        stats[nStats]->precipitation = (float) dades.precipitation;

        nStats++;
    }
}


void Lloyd(void){

    stats = NULL;
    datum = NULL;
    nStats = 0;

    signal(SIGINT, sigHandlerLloyd);
    signal(SIGALRM, sigHandlerLloyd);
    alarm(TIMETOW);

    initializeStats();
    initializeSharedMemory();

    Data aux;
    while(1){
        isAlarm = 1;
        while(isAlarm) {
            isAlarm = 0;
            SEM_wait(&s_lloyd);
        }

        strcpy_s(aux.name, datum->name);
        aux.temperature = datum->temperature;
        aux.humidity = datum->humidity;
        aux.pressure = datum->pressure;
        aux.precipitation = datum->precipitation;
        SEM_signal(&s_jack);

        processData(aux);
    }

}