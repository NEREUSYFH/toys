#include <iostream>
#include <functional>
#include <memory>
#include <queue>

class task_base {
public :
	virtual void run() = 0;
};
std::queue< std::shared_ptr<task_base> > q;
template<class Fn>
class task : public task_base{
public :
	task(Fn&& fn) : __fn(std::forward<Fn>(fn)) {}
	void run(){__fn();}
private :
	Fn __fn;
};

void fun(int x) {
	std::cout << x << std::endl;
}

template<class Fn>
std::shared_ptr<task<Fn>> make_task(Fn&& fn) {
	return  std::make_shared<task<Fn>>(std::forward<Fn>(fn));
}
template<class Fn>
void async(Fn&& fn) {
	q.push(make_task(std::forward<Fn>(fn)));
}
void run() {
	while(!q.empty()) {
		std::shared_ptr<task_base> p = q.front();
		q.pop();
		p->run();
	}
}
int main() {
	for (int i=0; i < 10; i ++) {
		async(std::bind(fun,i));
		printf("async fun %d\n", i);
	}
	run();	
}
