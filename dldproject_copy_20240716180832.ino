#include <LiquidCrystal_I2C.h>

// Pin definitions
const int moistureSensorPin = A0;     // Analog pin A0 for moisture sensor

LiquidCrystal_I2C lcd(0x27, 16, 2); // Adjust the address (0x27) based on your LCD module

float temperature = 0.0; // Placeholder for temperature
float humidity = 0.0; // Placeholder for humidity

int displayMode = 0; // 0: Temperature, 1: Humidity, 2: Soil Moisture
unsigned long displayTimer = 0;

// Task structure
struct Task {
    int priority;
    void (*action)();
    Task* next;
};

// Priority Queue implemented as a binary heap
class PriorityQueue {
private:
    Task** heap;
    int capacity;
    int size;

    void swap(Task*& a, Task*& b) {
        Task* temp = a;
        a = b;
        b = temp;
    }

    void heapifyUp(int index) {
        if (index && heap[parent(index)]->priority > heap[index]->priority) {
            swap(heap[index], heap[parent(index)]);
            heapifyUp(parent(index));
        }
    }

    void heapifyDown(int index) {
        int left = leftChild(index);
        int right = rightChild(index);
        int smallest = index;

        if (left < size && heap[left]->priority < heap[smallest]->priority) {
            smallest = left;
        }

        if (right < size && heap[right]->priority < heap[smallest]->priority) {
            smallest = right;
        }

        if (smallest != index) {
            swap(heap[index], heap[smallest]);
            heapifyDown(smallest);
        }
    }

    int parent(int index) { return (index - 1) / 2; }
    int leftChild(int index) { return (2 * index + 1); }
    int rightChild(int index) { return (2 * index + 2); }

public:
    PriorityQueue(int cap) : capacity(cap), size(0) {
        heap = new Task*[capacity];
    }

    void push(Task* task) {
        if (size == capacity) {
            Serial.println("Heap overflow");
            return;
        }
        heap[size] = task;
        heapifyUp(size);
        size++;
    }

    Task* pop() {
        if (size == 0) {
            return nullptr;
        }

        Task* root = heap[0];
        heap[0] = heap[--size];
        heapifyDown(0);

        return root;
    }

    bool empty() {
        return size == 0;
    }
};

// Simple linked list for storing historical sensor data
struct Node {
    float data;
    Node* next;
    Node(float d) : data(d), next(nullptr) {}
};

class LinkedList {
private:
    Node* head;
    int capacity;
    int size;

public:
    LinkedList(int cap) : head(nullptr), capacity(cap), size(0) {}

    void add(float data) {
        Node* newNode = new Node(data);
        newNode->next = head;
        head = newNode;
        size++;
        if (size > capacity) {
            removeLast();
        }
    }

    void removeLast() {
        if (!head) return;
        if (!head->next) {
            delete head;
            head = nullptr;
            size--;
            return;
        }

        Node* secondLast = head;
        while (secondLast->next->next) {
            secondLast = secondLast->next;
        }

        delete secondLast->next;
        secondLast->next = nullptr;
        size--;
    }

    void print() {
        Node* temp = head;
        while (temp) {
            Serial.print(temp->data);
            Serial.print(" -> ");
            temp = temp->next;
        }
        Serial.println("NULL");
    }
};

LinkedList temperatureHistory(10); // Placeholder
LinkedList humidityHistory(10); // Placeholder
PriorityQueue taskQueue(10);

void readSensors() {
    int moistureValue = analogRead(moistureSensorPin);
    int moisturePercentage = map(moistureValue, 0, 1023, 0, 100);

    temperatureHistory.add(temperature); // Placeholder
    humidityHistory.add(humidity); // Placeholder

    Serial.print("Soil Moisture: ");
    Serial.print(moisturePercentage);
    Serial.println("%");

    Serial.print("Temperature History: ");
    temperatureHistory.print();
    Serial.print("Humidity History: ");
    humidityHistory.print();
}

void displayStatus() {
    lcd.setCursor(0, 0);
    switch (displayMode) {
        case 0:
            lcd.print("Temp: ");
            lcd.print(temperature);
            lcd.print("C");
            break;
        case 1:
            lcd.print("Humidity: ");
            lcd.print(humidity);
            lcd.print("%");
            break;
        case 2:
            int moistureValue = analogRead(moistureSensorPin);
            int moisturePercentage = map(moistureValue, 0, 1023, 0, 100);
            lcd.print("Soil Moist: ");
            lcd.print(moisturePercentage);
            lcd.print("%");
            break;
    }

    lcd.setCursor(0, 1);
    switch (displayMode) {
        case 0:
            lcd.print("Next: Humidity");
            break;
        case 1:
            lcd.print("Next: Soil Moist");
            break;
        case 2:
            lcd.print("Next: Temp");
            break;
    }
}

void setupTasks() {
    Task* readSensorTask = new Task{1, readSensors, nullptr};
    Task* displayStatusTask = new Task{2, displayStatus, nullptr};

    taskQueue.push(readSensorTask);
    taskQueue.push(displayStatusTask);
}

void executeTasks() {
    while (!taskQueue.empty()) {
        Task* task = taskQueue.pop();
        task->action();
        delete task;
    }
}

void setup() {
    Serial.begin(9600);
    lcd.begin(16, 2);
    lcd.backlight();
}

void loop() {
    unsigned long currentMillis = millis();

    if (currentMillis - displayTimer >= 5000) {
        displayMode = (displayMode + 1) % 3;
        displayTimer = currentMillis;
    }

    setupTasks();
    executeTasks();

    delay(2000);  // Delay between readings
}
