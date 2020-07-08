#ifndef VIEW_ODSO9414DS
#define VIEW_ODSO9414DS

#include <vector>

namespace MaLib {

    template<class T>
    class View {
        const std::vector<T>& obj;
        unsigned int start;
        unsigned int stop;
        unsigned int step;

        class iterator {
            View* ptr;
            unsigned int pos;
        public:
          iterator(View * ptr, unsigned int pos): ptr(ptr), pos(pos) {

          }
          iterator operator++() {
              ++pos;
              return *this;
          }
          bool operator!=(const iterator & other) const {
              assert(ptr == other.ptr);
              return pos != other.pos;
          }
          const T& operator*() const {
              return ptr->operator[](pos);
          }
        };

    public:
        View(const std::vector<T>& obj, unsigned int start=0, int stop=-1, unsigned int step=1)
            : obj(obj), start(start), stop(static_cast<unsigned int>(stop)), step(step) {

        }

        View(const View<T>& v, unsigned int start=0, int stop=-1, unsigned int step=1)
            : obj(v.obj), start(v.start + start*v.step), stop(v.start + v.step*static_cast<unsigned int>(stop)), step(step*v.step) {

            if(stop < 0)
                this->stop = static_cast<unsigned int>(-1);
        }

        auto front() const {
            return obj[start];
        }

        const T& operator[] (unsigned int id) const {
            assert(id < obj.size());
            return obj[start + id*step];
        }

        unsigned int size() const {
            int taille = static_cast<int>(std::min(static_cast<unsigned int>(obj.size()), static_cast<unsigned int>(stop))) - static_cast<int>(start);

            if(taille <= 0)
                return 0;

            return static_cast<unsigned int>(1 + static_cast<unsigned int>(taille-1)/step);
        }

        std::vector<T> toVector() const {
            std::vector<T> result;
            for(unsigned int i=start; i<obj.size(); i+=step) {
                if(i==stop)
                    break;
                result.push_back(obj[i]);
            }
            return result;
        }

        void toVector(std::vector<T> &vec) const {
            for(unsigned int i=start; i<obj.size(); i+=step) {
                if(i==stop)
                    break;
                vec.push_back(obj[i]);
            }
        }


        iterator begin()  {
            return iterator(this, 0);
        }
        iterator end()  {
            return iterator(this, size());
        }

    };
}


#endif
