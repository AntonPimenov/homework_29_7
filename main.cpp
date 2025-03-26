#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

struct Node {
    int value;
    Node* next;
    std::mutex* node_mutex;

    Node(int val) : value(val), next(nullptr), node_mutex(new std::mutex()) {}
    ~Node() { delete node_mutex; }
};

class FineGrainedQueue {
public:
    FineGrainedQueue() : head(nullptr), queue_mutex(new std::mutex()) {}
    ~FineGrainedQueue() {
        Node* current = head;
        while (current) {
            Node* next = current->next;
            delete current;
            current = next;
        }
        delete queue_mutex;
    }

    // Вставка в позицию pos (начиная с 1)
    void insertIntoMiddle(int value, int pos) {
        Node* newNode = new Node(value);

        // Захватываем мьютекс очереди для безопасного доступа к head
        std::unique_lock<std::mutex> qlock(*queue_mutex);

        if (!head) {
            head = newNode;
            return;
        }

        Node* prev = head;
        prev->node_mutex->lock(); // Захватываем первый узел

        qlock.unlock(); // Больше не нужен, так как работаем с узлами

        Node* current = prev->next;
        int currPos = 1; // Позиция current (начинаем с 1, так как prev = head)

        // Идем до нужной позиции или конца списка
        while (current != nullptr && currPos < pos) {
            current->node_mutex->lock(); // Захватываем следующий узел

            // Перемещаемся дальше
            prev->node_mutex->unlock();
            prev = current;
            current = current->next;
            currPos++;
        }

        // Вставляем newNode между prev и current
        newNode->next = current;
        prev->next = newNode;

        // Разблокируем оставшиеся мьютексы
        prev->node_mutex->unlock();
        if (current) {
            current->node_mutex->unlock();
        }
    }

    // Вывод списка (не потокобезопасный, только для демонстрации)
    void printList() {
        Node* current = head;
        while (current) {
            std::cout << current->value << " -> ";
            current = current->next;
        }
        std::cout << "nullptr" << std::endl;
    }

private:
    Node* head;
    std::mutex* queue_mutex;
};

// Функция для тестирования в многопоточной среде
void testInsert(FineGrainedQueue& queue, int value, int pos) {
    queue.insertIntoMiddle(value, pos);
}

int main() {
    FineGrainedQueue queue;

    // Инициализация списка: 1 -> 3 -> 5
    queue.insertIntoMiddle(1, 1);
    queue.insertIntoMiddle(3, 2);
    queue.insertIntoMiddle(5, 3);

    std::cout << "Initial list: ";
    queue.printList(); // 1 -> 3 -> 5 -> nullptr

    // Вставка в середину (позиция 2)
    queue.insertIntoMiddle(10, 2);
    std::cout << "After inserting 10 at pos 2: ";
    queue.printList(); // 1 -> 10 -> 3 -> 5 -> nullptr

    // Вставка в конец (позиция 100)
    queue.insertIntoMiddle(20, 100);
    std::cout << "After inserting 20 at pos 100: ";
    queue.printList(); // 1 -> 10 -> 3 -> 5 -> 20 -> nullptr

    // Тест в многопоточной среде
    std::vector<std::thread> threads;
    threads.emplace_back(testInsert, std::ref(queue), 30, 2);
    threads.emplace_back(testInsert, std::ref(queue), 40, 4);
    threads.emplace_back(testInsert, std::ref(queue), 50, 10);

    for (auto& t : threads) {
        t.join();
    }

    std::cout << "After multithreaded inserts: ";
    queue.printList(); // Результат зависит от порядка выполнения, например: 1 -> 30 -> 10 -> 40 -> 3 -> 5 -> 20 -> 50 -> nullptr

    return 0;
}

