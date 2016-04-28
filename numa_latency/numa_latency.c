#include <numa.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include "mgen.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int from_node;
int target_node;

void* get_latency(void* data)
{
  // printf("from_node:%d target_node:%d\n", from_node, target_node);
    double latency = get_numa_latency(from_node, target_node);
    *((double*)data) = latency;
    return ((void*)0);
}

int get_node_first_cpu(int node)
{
	int i, err;
	struct bitmask *cpus;

	cpus = numa_allocate_cpumask();
	err = numa_node_to_cpus(node, cpus);
	if (err >= 0) {
		for (i = 0; i < cpus->size; i++)
			if (numa_bitmask_isbitset(cpus, i)){
				numa_free_cpumask(cpus);
				return i;
			}
	}
	numa_free_cpumask(cpus);
	return 0;
}

int main(void)
{
	int i = 0, j, ret;
        if(numa_available() < 0){
                printf("System does not support NUMA API!\n");
        }
        int n = numa_max_node();
	int *cpu_set = malloc(sizeof(int)*(n+1));

	double **latency_matrix = malloc(sizeof(double*)*(n+1));
	for(i = 0; i <= n; i++)
		latency_matrix[i] = malloc(sizeof(double)*(n+1));

        printf("There are %d nodes on your system\n", n + 1);
	for(i = 0; i <= n; i++) {
		cpu_set[i] = get_node_first_cpu(i);
	}
	int status;
	pthread_t tid;
	void* data = malloc(sizeof(double));
	for(i = 0; i <= n; i++) {
	//for(i = n; i >= 0; i--) {
		/* from node is the numa node number */
		//from_node = cpu_set[i];
		from_node = i;
		for(j = 0; j <= n; j++) {
			/* target node is the cpu node number */
			target_node = cpu_set[j];
			ret = pthread_create(&tid, NULL, get_latency , data);
			if (ret != 0)
    			{
      				printf("Create new thread failed: %s\n", strerror(ret));
      				exit(1);
    			}
			pthread_join(tid, NULL);
			latency_matrix[i][j] = *((double*)data);
			//printf("latency:%f\n", latency_matrix[i][j]);
		}
	}
	for(i = 0; i <= n; i++) {
		for(j = 0; j <= n; j++) {
			int to_cpu = cpu_set[j];
			printf("%10.2f ", latency_matrix[i][j]);
			//printf("to_cpu:%d latency:%f\n", to_cpu, latency_matrix[i][j]);
		}
		printf("\n");
	}
	for(i = 0; i <= n; i++)
		free(latency_matrix[i]);
	free(latency_matrix);
	free(cpu_set);
	free(data);
        return 0;
}
