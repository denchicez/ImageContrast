#include <fstream>
#include <string>
#include <utility>
#include "omp.h"
#pragma GCC optimize("O3")
const long long chank_sz = 2048;
unsigned char refactorPixel(double value, double minimum, double maximum){
    if(value>=maximum){
        return 255;
    }
    else if(value<minimum){
        return 0;
    }
    else return (value-minimum)/(maximum-minimum)*255;
}
std::pair<unsigned char , unsigned char > k_static(short layer, int height, int width, double k, unsigned char* matrix, short layers, unsigned long long sz_arr){
    double pixels_cut = height*width*k;
    int pixels_counter[256] = {0};
    unsigned char ansMin;
    unsigned char ansMax;
    long long counter = 0;
    #pragma omp parallel for schedule(static) reduction(+: pixels_counter)
    for (unsigned long long i = 0; i < sz_arr; i+=layers){
        pixels_counter[matrix[i+layer]]++;
    }
    for(int i = 0 ; i<=255; i++){ // parallel is bad
        counter+=pixels_counter[i];
        if(counter>pixels_cut){
            ansMin = i;
            break;
        }
    }
    counter = 0;
    for(int i = 255 ; i>=0; i--){ // parallel is bad
        counter+=pixels_counter[i];
        if(counter>pixels_cut){
            ansMax = i;
            break;
        }
    }
    delete[] pixels_counter;
    return std::make_pair(ansMin, ansMax);
}
int main(int argc, char *argv[]) {
    if(argc!=5){
        printf("Error. Need 5 argument");
        return 0;
    }
    int width, height, colorPolitre;
    short layers=0;
    try{
        std::ifstream filein;
        std::ofstream fileout;
        std::string inputFile = argv[2];
        std::string outputFile = argv[3];
        std::string str;
        double coef = atof(argv[4]);
        unsigned char paramMax=0;
        unsigned char paramMin=255;
        int num_threads = atof(argv[1]);
        delete argv;
        try{
            if(num_threads==0) num_threads = omp_get_max_threads();
            omp_set_num_threads(num_threads);
            try{
                filein.open(inputFile, std::fstream::in | std::fstream::binary);
                fileout.open(outputFile, std::fstream::out | std::fstream::binary);
                filein >> str;
                fileout << str << '\n';
                //cout << str << '\n';
                if(str=="P6"){
                    layers = 3;
                } else if (str=="P5"){
                    layers = 1;
                } else{
                    printf("file isn't correct for our task");
                    return 0;
                }
                filein >> width;
                fileout << width << '\n';
                //std::cout << "WIDTH : " << width << '\n';
                filein >> height;
                fileout << height << '\n';
                //std::cout << "HEIGHT : " << height << '\n';
                filein >> colorPolitre;
                fileout << colorPolitre << '\n';
                if(colorPolitre!=255){
                    printf("Problem with file. Tz hate this!");
                    return 0;
                }
                //std::cout << "COLOUR COUNTER : " << colorPolitre << '\n';
                unsigned long long const sz_arr = width*height*layers;
                unsigned char *matrix = new unsigned char[sz_arr];
                filein.read((char*) matrix, 1);
                filein.read((char*) matrix, sz_arr);
                filein.close();

                double start_time;
                start_time = omp_get_wtime();

                #pragma omp parallel for schedule(static) reduction(max: paramMax) reduction(min: paramMin)
                for(short layer = 0; layer < layers; layer++){
                    std::pair<unsigned char, unsigned char> minimumAndMaximum = k_static(layer, height, width, coef, matrix, layers, sz_arr);
                    //std::cout << "layer of " << layer << " have max is " << int(minimumAndMaximum.second) << " AND min is " << int(minimumAndMaximum.first) << '\n';
                    paramMax = std::max(minimumAndMaximum.second, paramMax);
                    paramMin = std::min(minimumAndMaximum.first, paramMin);
                }

                //std::cout << "layers active : " << layers << '\n';
                //std::cout << "Colour min coef is " << int(paramMin) << '\n';
                //std::cout << "Colour max coef is " << int(paramMax) << '\n';
                unsigned char *convert = new unsigned char[256];
                for(long long i = 0; i < 256; i++){
                    convert[i] = refactorPixel(i, paramMin, paramMax);
                }
                #pragma omp parallel for schedule(static)
                for(long long i = 0 ; i < sz_arr; i++){
                    matrix[i] = convert[matrix[i]];
                }
                delete[] convert;
                double end_time;
                end_time = omp_get_wtime();
                printf("Time (%i thread(s)): %g ms\n", num_threads, (end_time - start_time)*1000);
                fileout.write((char*)matrix, sz_arr);
                delete[] matrix;
                fileout.close();
            } catch(int e){
                printf("Can't open file or not found file or problem with img");
                return 0;
            }
        } catch(int e){
            printf("Problem with threads");
            return 0;
        }
    } catch(int e){
        printf("Problem with arguments");
        return 0;
    }
}
