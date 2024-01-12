#include <iostream>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <vector>
#include <string>
#include <string.h>
#include <sstream>
#include <assert.h>

using namespace std;

int main(int argc, char *argv[])
{
  try
  {
    if (argc != 4)
    {
      cout << "usage: " << argv[0] << " file1 file2 decimales" << endl;
      return 1;
    }

    string file1 = argv[1];
    string file2 = argv[2];
    int decimales = atoi(argv[3]);

    fstream File1(file1, ios_base::in);
    fstream File2(file2, ios_base::in);

    string tmp1, tmp2;
    unsigned long int count = 0;

    while (getline(File1, tmp1))
    {
      getline(File2, tmp2);
      count++;
      if (tmp1 != tmp2) {
        //Tokenizing
        stringstream ss1(tmp1), ss2(tmp2);
        string s1, s2;
        int third = 0;
        bool equal = false;
        while(getline(ss1,s1,',') && getline(ss2,s2,',')) {
          third++;
          //El tercero es el valor del profile
          if(third == 3) {
            double d1, d2;
            d1 = stod(s1);
            d2 = stod(s2);
            /*cout << s1 << " " << s2 << endl;
            cout << setprecision(numeric_limits<double>::max_digits10) << d1 << " " << d2 << endl;
            cout << trunc(d1*pow(10,decimales)) << " " << trunc(d2*pow(10,decimales)) << endl;*/
            if (trunc(d1*pow(10,decimales)) == trunc(d2*pow(10,decimales)) )
              equal = true;
            break;
          }
        }
        //Si count > 7 y equal es true quiere decir que estamos en la zona del
        //profile y los dos valores son iguales una vez aplicado el número de
        //decimales a comparar
        if( !(count > 7 && equal) ) {
          cout << count << " < " << tmp1 << endl;
          cout << count << " > " << tmp2 << endl;
          cout << "" << endl;
        }
      }
    }
    File1.close();
    File2.close();
  }
  catch (exception &e)
  {
    cout << "Exception: " << e.what() << endl;
  }
}
