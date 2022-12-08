#include <chrono>
#include <iostream>
#include <mutex>
#include <random>
#include <semaphore>
#include <thread>
#include "state.cpp"

#define MIN 400
#define MAX 800

constexpr const size_t N = 5;  // Quantity of philosophers.

size_t inline left(size_t i) { return (i + N - 1) % N; }  // It returns the left neighbour philosopher of philosopher i.

size_t inline right(size_t i) { return (i + 1) % N; }  // It returns the right neighbour philosopher of philosopher i.

State states[N];  // Each philosopher has a current state: THINKING, HUNGRY and EATING which is mapped by an enum class.

std::mutex critical_region_mtx;  // Mutex used to grant mutual exclusion in critical regions.
std::mutex output_mtx;  // Mutex used for synchronizing the output.

std::binary_semaphore both_forks_available[N] {  // One binary semaphore for each philosopher for controlling hold of the forks.
	std::binary_semaphore{0}, std::binary_semaphore{0},
    std::binary_semaphore{0}, std::binary_semaphore{0},
    std::binary_semaphore{0}
};

size_t get_random(size_t min, size_t max) {  // It generates and returns a size_t between min and max.
	static std::mt19937 rnd(std::time(nullptr));
	return std::uniform_int_distribution<>(min, max) (rnd);
}

void test_philosopher_may_eat(size_t i) {  // This function tests whether a philosopher may eat or not checking his corresponding state and his neighbours.
										   // It also avoids this implementation for falling onto a deadlock once there's a check before eating. 
	if (states[i] == State::HUNGRY && states[left(i)] != State::EATING && states[right(i)] != State::EATING) {
		states[i] = State::EATING;
		both_forks_available[i].release();
	}
}

void think(size_t i) {
	size_t duration = get_random(MIN, MAX); 
	{
		std::lock_guard<std::mutex> lk(output_mtx);  // Blocking the output_mtx from being used by another thread.
		std::cout << i << " thinks " << duration << " ms.\n"; 
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(duration));  // Blocks the execution of the current thread for at least the specified sleep_duration.
}

void take_forks(size_t i) {  // This function is liable for allowing the philosopher i for taking possession of the forks.
	{
		std::lock_guard<std::mutex> lk{critical_region_mtx};  // Blocking the critial_region_mtx from being used by another thread.
		states[i] = State::HUNGRY;
		{
			std::lock_guard<std::mutex> lk(output_mtx);  // Block the output_mtx from being used by another thread.
			std::cout << "\t\t" << i << " is hungry.\n";
		}
		test_philosopher_may_eat(i); // Tests whether the philosopher i may or may not take hold of his forks.
	}
	both_forks_available[i].acquire();  // This method tries to take hold of the shared resource. If it is not available, it may block until it becomes.
}

void eat(size_t i) {
	size_t duration = get_random(MIN, MAX);	
	{
		std::lock_guard<std::mutex> lk(output_mtx);  // Blocks the output_mtx from being used by another thread.
		std::cout << i << " eats " << duration << " ms.\n";
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(duration));
}

void put_forks(size_t i) {
	std::lock_guard<std::mutex> lk {critical_region_mtx};
	states[i] = State::THINKING;
	test_philosopher_may_eat(left(i));
	test_philosopher_may_eat(right(i));
}

void execute_for_philosopher(size_t i) {
	while (true) {
		think(i);
		take_forks(i);
		eat(i);
		put_forks(i);
	}
}

int main() {
	std::cout << "WELCOME TO THE DINNING PHILOSOPHERS PROBLEM SOLVED WITH ONE MUTEX, ONE SEMAPHORE AND STATE VARIABLE PER PHILOSOPHER.\n";
	std::jthread t0([&] {execute_for_philosopher(0); });
	std::jthread t1([&] {execute_for_philosopher(1); });
	std::jthread t2([&] {execute_for_philosopher(2); });
	std::jthread t3([&] {execute_for_philosopher(3); });
	std::jthread t4([&] {execute_for_philosopher(4); });

	return 0;
}

