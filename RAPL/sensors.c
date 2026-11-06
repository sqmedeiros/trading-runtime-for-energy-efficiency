#include "sensors.h"

float getTemperature() {
    char command[100];
    char output[200];
    float temperature, mean_temp = 0.0, tctl_temp = 0.0;
    int count = 0, tctl_count = 0;

    // Run sensors command and capture output
    // Run sensors command and capture output
    int status = system("sensors > temp.txt");
    if (status != 0) {
        perror("Error executing command: sensors > temp.txt");
        printf("STATUS:%d \n", status);
        return -1;
    }

    FILE* temp_file = fopen("temp.txt", "r");
        
    // Parse temperature from output
    while(fgets(output, 200, temp_file)) {
        if(strstr(output, "Core")) { //TODO mudar isto maybe
            sscanf(output, "Core %*d: +%f°C", &temperature);
            mean_temp += temperature;
            count++;
        } else if(!strncmp(output, "Tctl:", 5)) { // AMD has no per-core sensors
            sscanf(output, "Tctl: %f", &temperature);
            tctl_temp += temperature;
            tctl_count++;
        }
    }

    if (count > 0) {
      mean_temp /= count;
		} else if (tctl_count > 0) {
      mean_temp = tctl_temp / tctl_count;
    }
        
    fclose(temp_file);
    remove("temp.txt");
    printf("\nMean temperature in all cores is %.10f°C\n", mean_temp);

    return mean_temp;
}
