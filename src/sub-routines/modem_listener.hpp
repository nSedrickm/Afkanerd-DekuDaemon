#include <map>
#include <vector>
#ifndef MODEM_LISTENER_H_INCLUDED_
#define MODEM_LISTENER_H_INCLUDED_
using namespace std;

class Modem {
	string index;
	string isp;
	string imei;
	public:
		Modem();

		void setIndex( string index );
		void setIMEI( string IMEI );
		void setISP( string ISP );

		string getIndex();
		string getISP() const;
		string getIMEI();

		explicit operator bool() const;
};

class Modems {
	vector<Modem> modemCollection;
	public:
		Modems();
		void __INIT__();
		
		vector<string> getAllIndexes();
		vector<string> getAllISP();
		vector<string> getAllIMEI();

		vector<Modem> getAllModems();
};

#endif
