#include <iostream>
#include <array>
#include <random>
#include <thread> // para los hilos
#include <mutex> // para el semáforo
#include <chrono>
#include <cstdlib> // contiene el prototipo de las funciones srand y rand
using namespace std;

const int N = 25; // capacidad del contenedor
int VECES_INSERTAR = 4;
int VECES_SACAR = 3;

mutex semProductor; // semaforo binario del productor
mutex semConsumidor; // semaforo binario del consumidor
mutex semImprimir; // semaforo para imprimir en la consola

// generar numeros aleatorios
random_device rd;
mt19937 mt(rd());
uniform_int_distribution<uint32_t> uint_aleatorio(1000, 3000);


uniform_int_distribution<uint32_t> uint_distchar(0, 500);

class Contenedor
{
    public:
        Contenedor()
        {
            this->tamanio = 0;
        }

        string eliminar(int pos)
        {
            string aux = contenedor[pos];
            contenedor[pos] = "";
            tamanio--;
            return aux;
        }

        void insertar(int pos, string nuevo)
        {
            contenedor[pos] = nuevo;
            tamanio++;
        }

        string at(int pos) { return contenedor.at(pos); }
        int getTamanio() { return tamanio; }

    private:
        array<string, N> contenedor;
        int tamanio;
};


Contenedor contenedor;

void imprimirContenedor()
{
    semImprimir.lock();
    cout << "[ ";
    for(int i = 0; i < N; i++) {
        cout << contenedor.at(i);

        if(i + 1 < N)
            cout << ", ";
    }
    cout << "]" << endl;
    semImprimir.unlock();
}

class Productor
{
    public:
        Productor()
        {
            this->posicion = 0;
        }

        void producir()
        {
            while(true)
            {   
                semImprimir.lock();
                cout << endl << "PRODUCTOR intenta ingresar" << endl;
                semImprimir.unlock();

                if(contenedor.getTamanio() < N)
                {
                    string nuevo = generarProducto();
                    semProductor.lock();
                    cout << endl << "PRODUCTOR ingreso" << endl;

                    int insertados = 0;
                    cout << "inserto producto en posiciones: "; 
                    for(int i = 0; i < VECES_INSERTAR && contenedor.getTamanio() < N; i++)
                    {
                        // insertar
                        string nuevo = generarProducto();
                        contenedor.insertar(posicion, nuevo);
                        insertados++;
                        cout << posicion << "   ";
                        posicion = (posicion + 1) % N;
                    }
                    cout << endl;

                    imprimirContenedor();

                    if(contenedor.getTamanio() == N) {
                        VECES_INSERTAR = 3;
                        VECES_SACAR = 4;
                    }

                    if( contenedor.getTamanio() == insertados ){ // si el contenedor estaba vacío
                        cout << endl << "CONSUMIDOR desperto" << endl;
                        semConsumidor.unlock();
                    }

                    semProductor.unlock();

                } else {
                    cout << "PRODUCTOR no pudo ingresar porque el contenedor esta LLENO" << endl;
                }

                // espera tiempo aleatorio
                int milisegundos = uint_aleatorio(mt);
                std::this_thread::sleep_for(std::chrono::milliseconds(milisegundos));

            }
        }

    private:
        int posicion;

        // genera un producro aleatorio
        string generarProducto()
        {
            int aleatorio = uint_distchar(mt);
            
            if(aleatorio >= 33 && aleatorio <= 126 && aleatorio != 44 && aleatorio != 46) {
                char letra = static_cast<char>(aleatorio);
                return string(1, letra);
            }

            return to_string(aleatorio);
        }
};

class Consumidor
{
    public:
        Consumidor()
        {
            this->posicion = 0;
        }

        void consumir()
        {
            int m;
            semImprimir.lock();
            cout << "CONSUMIDOR esta dormido" << endl;
            semImprimir.unlock();

            semConsumidor.lock(); // espera a que el productor inserte producto

            while(true)
            {
                semImprimir.lock();
                cout << endl << "CONSUMIDOR intenta ingresar" << endl;
                semImprimir.unlock();

                semProductor.lock(); // bloquea al productor
                cout << endl << "CONSUMIDOR ingreso" << endl;
                //string extraido = contenedor.eliminar(posicion);

                cout << "extrajo de las posiciones: ";
                for(int i = 0; i < VECES_SACAR && contenedor.getTamanio() > 0; i++)
                {
                    string extraido = contenedor.eliminar(posicion);
                    cout << posicion << "   ";
                    posicion = (posicion + 1) % N;
                }
                cout << endl;
                m = contenedor.getTamanio();
                imprimirContenedor();

                semProductor.unlock();

                if(m == 0){ // si el contendor esta vacío bloquea al consumidor
                    cout << endl << "CONSUMIDOR se fue a dormir" << endl;
                    semConsumidor.lock();
                }

                // espera tiempo aleatorio
                int milisegundos = uint_aleatorio(mt);
                std::this_thread::sleep_for(std::chrono::milliseconds(milisegundos));
            }
        }

    private:
        int posicion;
};




int main()
{

    // productor inicia primero que el consumidor
    semProductor.unlock();
    semConsumidor.lock();

    thread productor(&Productor::producir, new Productor());
    thread consumidor(&Consumidor::consumir, new Consumidor());

    productor.join();
    consumidor.join();
}
