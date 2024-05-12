#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include <time.h>
#include <chrono>
#include <iostream>
#include <fstream>

#define N 100


void merge(int* A, int sizeA, int* B, int sizeB) {
    int sizeC = sizeA + sizeB;
    int* C = (int*)malloc(sizeC * sizeof(int));
    int countA;
    int countB;
    int countC;
    

    for (countA = 0, countB = 0, countC = 0; countC < sizeC; countC++) {
        if (countA >= sizeA) {
            C[countC] = B[countB++];
        }
        else if (countB >= sizeB) {
            C[countC] = A[countA++]; 
        }
        else {
            if (A[countA] <= B[countB]) {
                C[countC] = A[countA++];
            }
            else {
                C[countC] = B[countB++];
            }
        }
    }

  
    for (countA = 0; countA < sizeA; countA++) {
        A[countA] = C[countA];
    }

    for (countC = countA, countB = 0; countC < sizeC; countC++, countB++){
        B[countB] = C[countC];     
    }
    free(C);
}


void mergeSort(int* data, int startPoint, int endPoint)
{
    int middle = (startPoint + endPoint) / 2;
    if (startPoint == endPoint)
        return;
    mergeSort(data, startPoint, middle);
    mergeSort(data, middle + 1, endPoint);
    merge(data + startPoint, middle - startPoint + 1, data + middle + 1, endPoint - middle);
}


void printArray(int* data)
{
    for (int i = 0; i < N; i++) {
        printf("%d ", data[i]);
    }
    printf("\n");

   
}

void printArrayFile(int* data, const char* filename)
{
    FILE* file;
    if ((file = fopen(filename, "w")) == NULL) {
        printf("Невозможно открыть файл\n");
        exit(1);
    }
    else {
        for (int i = 0; i < N; ++i) {
            fprintf(file, "%d\t", data[i]);
        };
        printf("\n");
    }
    fclose(file);
}




int main(int argc, char* argv[])
{
    int i;
    int* data = NULL;  
    int scale = 0;
    int currentLevel = 0;    
    int maxLevel = 0; 
    int middle = 0;     
    int length = 0;   
    int rightLength;
    int p;    
    int mpiRank; 
    MPI_Status status;
    double t1, t2, dt;
    
   

    MPI_Init(&argc, &argv);
    t1 = MPI_Wtime();
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);

    if (mpiRank == 0) {
        maxLevel = log((double)p) / log(2.00); 
        length = N;  
        data = (int*)malloc(length * sizeof(int)); 
        for (i = 0; i < N; i++) {
            data[i] = rand() % 100;
        }
       
        //printArray(data);
        //printf("Array before sort: \n");
        printArrayFile(data, "input.txt");
    }

   
    MPI_Bcast(&maxLevel, 1, MPI_INT, 0, MPI_COMM_WORLD); //maxLevel->other P

    for (currentLevel = 0; currentLevel <= maxLevel; currentLevel++) {
        scale = pow(2.00, currentLevel);
        if (mpiRank / scale < 1) { //родительский узел
            if ((mpiRank + scale) < p) { //если дочерний узел существует
                middle = length / 2; 
                rightLength = length - middle; 
                length = middle; 
                MPI_Send(&rightLength, 1, MPI_INT, mpiRank + scale, currentLevel, MPI_COMM_WORLD); // отправляем длину дочернего узла 
                MPI_Send((int*)(data + middle), rightLength, MPI_INT, mpiRank + scale, currentLevel, MPI_COMM_WORLD); //отправляем правую часть
            } 
        }
        else if (mpiRank / scale < 2) { //дочерний узел
            MPI_Recv(&length, 1, MPI_INT, mpiRank - scale, currentLevel, MPI_COMM_WORLD, &status); //получаем длину из родительского узла
            data = (int*)malloc(length * sizeof(int));    
            MPI_Recv(data, length, MPI_INT, mpiRank - scale, currentLevel, MPI_COMM_WORLD, &status);//получаем массив от родительского узла
        }
    }


   
    mergeSort(data, 0, length - 1);      
  
    for (currentLevel = maxLevel; currentLevel >= 0; currentLevel--) {
        scale = pow(2.00, currentLevel);
        if (mpiRank / scale < 1) {             // Родительский узел получает отсортированный массив из дочернего узла      
            if (mpiRank + scale < p) {            // Если дочерний узел существует (ранг дочернего узла <количество процессоров)                 
                MPI_Recv(&rightLength, 1, MPI_INT, mpiRank + scale, currentLevel, MPI_COMM_WORLD, &status);
                MPI_Recv((int*)data + length, rightLength, MPI_INT, mpiRank + scale, currentLevel, MPI_COMM_WORLD, &status);
                merge(data, length, (int*)data + length, rightLength);      
                length += rightLength; 
            }
        }     
        else if (mpiRank / scale < 2) {       // Дочерний узел посылает отсортрованный массив в родительский   
            MPI_Send(&length, 1, MPI_INT, mpiRank - scale, currentLevel, MPI_COMM_WORLD);
            MPI_Send(data, length, MPI_INT, mpiRank - scale, currentLevel, MPI_COMM_WORLD);
        }
    }

    
    if (mpiRank == 0) {
		//printf("Sorted Data: \n");
        //printArray(data);
        t2 = MPI_Wtime();
        printArrayFile(data, "output.txt");
        printf("\nTime taken: %f s\n", t2 - t1);
    }

   

   
    //printf("\nt2: %f s\n", t2 );
    
    MPI_Finalize();
    free(data); 
    return 0;
}

   

    