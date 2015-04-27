#include "NameSpace.h"
#include "iostream"

using namespace std;
using namespace glsp::ogl;

int main(void)
{
	unsigned names[2];
	NameItem nameItems[2];
	bool ret;

	cout << "NameItem 0 is " << &nameItems[0] << endl;
	cout << "NameItem 1 is " << &nameItems[1] << endl;
	NameSpace *pNameSpace = new NameSpace();

	ret = pNameSpace->genNames(2, names);
	if(!ret)
		cout << "genNames failed!" << endl;

	nameItems[0].setName(names[0]);
	nameItems[1].setName(names[1]);

	if(!pNameSpace->retrieveObject(names[0]))
	{
		ret = pNameSpace->insertObject(&nameItems[0]);
		if(!ret)
			cout << "retrieveObject 0 failed!" << endl;
	}
	if(!pNameSpace->retrieveObject(names[1]))
	{
		ret = pNameSpace->insertObject(&nameItems[1]);
		if(!ret)
			cout << "retrieveObject 1 failed!" << endl;
	}

	NameItem *pNameItem0 = pNameSpace->retrieveObject(names[0]);
	cout << "Retrieve NameItem 0 is " << pNameItem0 << endl;
	NameItem *pNameItem1 = pNameSpace->retrieveObject(names[1]);
	cout << "Retrieve NameItem 1 is " << pNameItem1 << endl;

	ret = pNameSpace->removeObject(pNameItem0);
	if(!ret)
		cout << "removeObject 0 failed!" << endl;

	ret = pNameSpace->removeObject(pNameItem1);
	if(!ret)
		cout << "removeObject 1 failed!" << endl;

	ret = pNameSpace->deleteNames(2, names);
	if(!ret)
		cout << "deleteNames failed!" << endl;

	return 0;
}
