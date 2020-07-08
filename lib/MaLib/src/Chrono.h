#ifndef CHRONO_R7U52KTM

#define CHRONO_R7U52KTM

#include <sys/time.h>
#include <cstdio>
#include <iostream>


namespace MaLib
{
    class Chrono
    {
        public :

            Chrono(std::string name, bool afficherQuandDetruit=true)
                : _name(name), _duree(0),_dureeSec(0), _pause(false), _afficherQuandDetruit(afficherQuandDetruit)
            {
                gettimeofday(&depart, &tz);
            }

            Chrono()
                : _duree(0),_dureeSec(0), _pause(false)
            {
                gettimeofday(&depart, &tz);
            }

            ~Chrono() {
                if(_name.size())
                    if(_afficherQuandDetruit)
                        print();
            }

            void setDuree(long sec, long microSec=0)
            {
                _duree= sec * 1000000L + microSec;
                _dureeSec=sec;
            }

            void tic()
            {
                _pause=false;
                _duree=0;
                _dureeSec=0;
                gettimeofday(&depart, &tz);
            }

            long pause(bool val)
            {
                if(val)
                {
                    if(!_pause)
                    {
                        gettimeofday(&fin, &tz);
                        _duree += (fin.tv_sec-depart.tv_sec) * 1000000L + (fin.tv_usec-depart.tv_usec);
                        _dureeSec += fin.tv_sec-depart.tv_sec ;
                        _pause=true;
                    }
                }else
                {
                    if(_pause)
                    {
                        gettimeofday(&depart, &tz);
                        _pause=false;
                    }
                }
                return _duree;
            }

            long pauseSec(bool val)
            {
                if(val)
                {
                    if(!_pause)
                    {
                        gettimeofday(&fin, &tz);
                        _duree += (fin.tv_sec-depart.tv_sec) * 1000000L + (fin.tv_usec-depart.tv_usec);
                        _dureeSec += fin.tv_sec-depart.tv_sec ;
                        _pause=true;
                    }
                }else
                {
                    if(_pause)
                    {
                        gettimeofday(&depart, &tz);
                        _pause=false;
                    }
                }
                return _dureeSec;
            }

            long tac()
            {
                if(_pause==false)
                {
                    gettimeofday(&fin, &tz);
                    return (fin.tv_sec-depart.tv_sec) * 1000000L + (fin.tv_usec-depart.tv_usec) + _duree;
                }else
                {
                    return _duree;
                }
            }

	    long tacSec()
            {
                if(_pause==false)
                {
                    gettimeofday(&fin, &tz);
                    return (fin.tv_sec-depart.tv_sec) + _dureeSec;
                }else
                {
                    return _dureeSec;
                }
            }

            void print()
            {
                double val = tac();

                if(_name.size())
                    std::cout << _name << ": ";
                if(val < 1000.0)
                    std::cout << val << " µs" << std::endl;
                else if(val < 1000000.0)
                    std::cout << val/1000.0 << " ms" << std::endl;
                else
                    std::cout << val/1000000.0 << " sec" << std::endl;
            }

        private :


        std::string _name;
        struct timeval depart, fin;
        struct timezone tz;
        long _duree;
        long _dureeSec;

        bool _pause;
        bool _afficherQuandDetruit;
    };
}


#endif /* end of include guard: CHRONO_R7U52KTM */




