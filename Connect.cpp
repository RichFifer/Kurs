/**
 * @file Connect.cpp
 */
#include "Connect.h"
#include "md5.h"
#include <cstring> // Для memset
#include <cstdlib> // Для rand
#include <ctime>   // Для time
/**
 * @brief Получение из файла логина и пароля.
*/
void Connect::GetLoginPassword(){
    // Открытие файла для аутентификации
    ifstream fa(name_auto_file);
    if(!fa.is_open()){
        fa.close();
        throw error_proj(std::string("fun:GetLoginPassword, param:name_auto_file.\nОшибка отрытия файла для аутентификации!"));
    }

    string line;
    if(fa.peek() == EOF){
        fa.close();
        throw error_proj(std::string("fun:GetLoginPassword, param:name_auto_file.\nОшибка отрытия файла для аутентификации! Файл пуст!"));
    }
    getline(fa, line);
    int k = line.find(" ");
    string u = line.substr(0, k);
    string p = line.erase(0, k+1);
    username = u;
    pswd = p;

}
/**
 * @brief Генерация соли
*/
string generateSalt() {
    const int SALT_LENGTH = 16;
    char salt[SALT_LENGTH + 1];
    srand(time(0)); // Инициализация генератора случайных чисел
    for (int i = 0; i < SALT_LENGTH; i++) {
        salt[i] = 'a' + rand() % 26; // Генерация случайного символа
    }
    salt[SALT_LENGTH] = '\0';
    return string(salt);
}
/**
* @brief Взаимодействие с сервером.
* @param str1 адрес сервера.
* @param str2 порт сервера.
* @throw error_proj при возникновении ошибки.
*/
int Connect::Connect_to_server(string str1, string str2) {
    GetLoginPassword();

    // Открытие файла для чтения векторов
    ifstream fin(name_original_file);
    if (!fin.is_open()) {
        fin.close();
        throw error_proj("fun:Connect_to_server, param:name_original_file.\nОшибка отрытия файла с векторами!");
    }

    string line;
    if (fin.peek() == EOF) {
        fin.close();
        throw error_proj("fun:Connect_to_server, param:name_original_file.\nОшибка отрытия файла. Файл с векторами пуст!");
    }

    // Открытие файла для записи суммы
    ofstream fout(name_result_file, ios_base::app);
    if (!fout.is_open()) {
        fout.close();
        throw error_proj("fun:Connect_to_server, param:name_result_file.\nФайл для результата не может быть открыт или создан!");
    }

    try {
        ip_addr = stoi(str1);
    } catch (...) {
        throw error_proj("fun:Connect_to_server, param:ip_addr.\nНе возможно получить адрес!");
    }

    try {
        port = stoi(str2);
    } catch (...) {
        throw error_proj("fun:Connect_to_server, param:port.\nНе возможно получить порт!");
    }

    char buf[1024];
    int bytes_read;

    int sock;
    struct sockaddr_in addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        throw error_proj("fun:Connect_to_server, param:sock.\nОшибка создания sock!");
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(ip_addr);
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        throw error_proj("fun:Connect_to_server, param:sock.\nОшибка подключения к серверу!");
    }

    // Генерация соли
    string salt = generateSalt();

    // Вычисление хэша-кода от SALT + PASSWORD
    string hash = MD5_hash(salt + pswd);

    // Формирование сообщения MSG = LOGIN || SALT || HASH
    string msg = username + salt + hash;

    // Отправка сообщения MSG
    send(sock, msg.c_str(), msg.length(), 0);

    // Получение ответа сервера
    bytes_read = recv(sock, buf, sizeof(buf), 0);
    buf[bytes_read] = '\0';
    string response = string(buf);

    if (response == "ERR") {
        close(sock);
        throw error_proj("fun:Connect_to_server, param:pswd.\nОшибка аутентификации!");
    }

    // Получение количества векторов
    getline(fin, line);
    int len = 0;
    try {
        len = stoi(line);
    } catch (...) {
        throw error_proj("fun:Connect_to_server, param:line.\nНе возможно получить количество векторов!");
    }
    fout << len << endl;

    // Отправка количества векторов
    send(sock, &len, sizeof(len), 0);

    // Отправка векторов
    for (int l = 0; l < len; l++) {
        getline(fin, line);

        // Получение длины вектора и его значений
        string s = line;
        int k = s.find(" ");
        string L = s.substr(0, k);
        s = s.erase(0, k + 1);
        int Len = 0;
        try {
            Len = stoi(L);
        } catch (...) {
            throw error_proj("fun:Connect_to_server, param:line.\nНе возможно получить размер вектора!");
        }

        float array[Len] = {0};
        for (int j = 0; j < Len; j++) {
            string a;
            int i = s.find(" ");
            a = s.substr(0, i);
            s = s.erase(0, i + 1);

            try {
                array[j] = stof(a);
            } catch (...) {
                throw error_proj("fun:Connect_to_server, param:s-строка.\nНе возможно получить вектор!");
            }
        }

        send(sock, &Len, sizeof(Len), 0);
        send(sock, array, sizeof(array), 0);
        float sum = 0;
        recv(sock, &sum, sizeof(sum), 0);

        fout << sum << endl;
    }

    // Закрытие файлов
    fin.close();
    fout.close();

    // Сохранение результатов в файле
    cout << "Результат сохранен в файле: " << name_result_file << endl;
    close(sock);

    return 0;
}
