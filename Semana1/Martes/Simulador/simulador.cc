#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>
#include <chrono>

using namespace std;

queue<string> colaClienteInter;
queue<string> colaInterServidor;
queue<string> colaServidorInter;

mutex m1, m2, m3;
condition_variable cv1, cv2, cv3;
bool terminado = false;

void cliente() {
    this_thread::sleep_for(chrono::seconds(1));
    string figura = "FIGURA1";
    cout << "El cliente solicita figura: " << figura << endl;

    {
        unique_lock<mutex> lock(m1);
        colaClienteInter.push("REQ " + figura);
    }
    cv1.notify_one();

    unique_lock<mutex> lock(m3);
    cv3.wait(lock, []{ return !colaServidorInter.empty(); });
    string respuesta = colaServidorInter.front();
    colaServidorInter.pop();

    cout << "El cliente recibe la respuesta: " << respuesta << endl;
    terminado = true;
    cv1.notify_all();
}

void intermediario() {
    while (true) {
        unique_lock<mutex> lock(m1);
        cv1.wait(lock, []{ return !colaClienteInter.empty() || terminado; });

        if (terminado && colaClienteInter.empty()) {
            break;
        }

        string mensaje = colaClienteInter.front();
        colaClienteInter.pop();

        cout << "El intermediario recibe la solicitud del cliente" << endl;
        cout << "El intermediario envía la solicitud al servidor" << endl;
        {
            unique_lock<mutex> lock2(m2);
            colaInterServidor.push(mensaje);
        }
        cv2.notify_one();
    }
}

void servidor() {
    unique_lock<mutex> lock(m2);
    cv2.wait(lock, []{ return !colaInterServidor.empty(); });
    string mensaje = colaInterServidor.front();
    colaInterServidor.pop();

    cout << "El servidor recibe la solicitud" << endl;
    cout << "El servidor busca la figura" << endl;

    this_thread::sleep_for(chrono::seconds(2));
    string respuesta = "RES FIGURA1 FOUND";
    cout << "El servidor responde al intermediario" << endl;
    {
        unique_lock<mutex> lock3(m3);
        colaServidorInter.push(respuesta);
    }
    cv3.notify_one();
}

int main() {
    thread t1(cliente);
    thread t2(intermediario);
    thread t3(servidor);
    t1.join();
    t2.join();
    t3.join();

    cout << "Fin de la simulación" << endl;

    return 0;
}