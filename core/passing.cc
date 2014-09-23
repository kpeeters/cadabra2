
#include <iostream>

int k=5;

void fun(int * &p)
	{
	p=&k;
	}

int main()
	{
	int i=3;
	int *p = &i;

	std::cout << *p << std::endl;
	fun(p);
	std::cout << *p << std::endl;
	}
