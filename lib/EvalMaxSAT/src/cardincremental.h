#pragma once


#include <optional>
#include <cassert>
#include <memory>
#include <deque>
#include <iostream>

#include "lazyvariable.h"

template<class T>
class CardIncremental_Lazy {

    T *solver;
    unsigned int nbLit;
    unsigned int bound;

   struct TotTree {
        // Each non-leaf node can have multiple vars who have impliquants
        std::vector< std::shared_ptr<LazyVariable<T>> > lazyVars;
        unsigned nof_input; // Number of literals under the node
        std::shared_ptr<TotTree> left;
        std::shared_ptr<TotTree> right;

        void print(std::ostream& os, bool first=true) const {
            if(left == nullptr && right == nullptr) {
                assert( lazyVars.size() == 1);
                if(!first)
                    os << ", ";
                os << lazyVars[0];
            } else {
                if(left != nullptr) {
                    left->print(os, first);
                }
                if(right != nullptr) {
                    right->print(os, false);
                }
            }
        }


        std::vector<int> getClause() {
            std::vector<int> clause;
            getClause(clause);
            return clause;
        }

   private:
       void getClause(std::vector<int> &clause) {
           if(left == nullptr && right == nullptr) {
               assert(lazyVars.size() == 1);
               clause.push_back( lazyVars[0]->get() );
           } else {
               if(left != nullptr) {
                   left->getClause(clause);
               }
               if(right != nullptr) {
                   right->getClause(clause);
               }
           }
       }

    };

    std::shared_ptr<TotTree> _tree;
    unsigned int _maxVars; // Max number of literals in a tree, ignoring k-simplification
public:


public:

    CardIncremental_Lazy(T *solver, const std::vector<int> &clause, unsigned int bound=0)
        : solver(solver), nbLit(static_cast<unsigned int>(clause.size())), bound(bound), _maxVars( clause.size())
    {
        std::deque<std::shared_ptr<TotTree>> nqueue;

        // Create the leafs and store them in a queue
        for ( unsigned i = 0; i < _maxVars; ++i) {
            std::shared_ptr<TotTree> node = std::make_shared<TotTree>();

            node->lazyVars.push_back( std::make_shared<LazyVariable<T>>(clause[i]));
            node->nof_input = 1;
            node->left      = nullptr;
            node->right     = 0;

            nqueue.push_back( node);
        }

        // Create non-leaf nodes from the bottom-up by starting from the beginning of the queue
        while (nqueue.size() > 1) {
            auto l = nqueue.front();
            nqueue.pop_front();
            auto r = nqueue.front();
            nqueue.pop_front();

            auto node = std::make_shared<TotTree>();
            node->nof_input = l->nof_input + r->nof_input;
            node->left      = l;
            node->right     = r;

            // Bound is the RHS. No need to represent more than RHS + 1 because of k-simplification
            unsigned kmin = std::min(bound + 1, node->nof_input);

            node->lazyVars.resize( kmin);
            for (unsigned i = 0; i < kmin; ++i)
                node->lazyVars[i] = std::make_shared<LazyVariable<T>>(solver);

            new_ua( node->lazyVars, kmin, l->lazyVars, r->lazyVars);
            nqueue.push_back(node);
        }

        _tree = nqueue.front();
    }

    ~CardIncremental_Lazy() {}

    int operator <= (unsigned int k) {
        return atMost(k);
    }

    unsigned int size() const {
        return nbLit;
    }

    std::vector<int> getClause() {
        return _tree->getClause();
    }


    void print(std::ostream& os) const {
        os << "[";
        _tree->print(os, true);
        os << "]";
    }

    void add(const std::vector<int>& clause) {
        CardIncremental_Lazy<T> tb(solver, clause, _tree->lazyVars.size() - 1);

        unsigned n    = _tree->nof_input + tb._tree->nof_input;
        unsigned kmin = n;
        if( _tree->lazyVars.size() < n)
            kmin = _tree->lazyVars.size();


        std::shared_ptr<TotTree> tree = std::make_shared<TotTree>();
        tree->nof_input = n;
        tree->left      = _tree;
        tree->right     = tb._tree;

        tree->lazyVars.resize( kmin);
        for (unsigned i = 0; i < kmin; ++i)
            tree->lazyVars[i] = std::make_shared<LazyVariable<T>>(solver);

        new_ua( tree->lazyVars, kmin, _tree->lazyVars, tb._tree->lazyVars);

        _maxVars += clause.size();
        _tree = tree;
    }


    int atMost(unsigned int k) {
        // if the bound is bigger or equal to the current bound
        if( k >= _maxVars ) {
            return 0;
        }

        // Increase node (add vars) if possible and needed
        if( k >= _tree->lazyVars.size() ) {
            increase(_tree, k);
        }
        assert(k < _tree->lazyVars.size());

        // Return the soft var corresponding to a cardinality constraint from the tree with a bound of k
        return -_tree->lazyVars[k]->get();
    }


private:

    void increase(std::shared_ptr<TotTree> tree, unsigned newBound)
    {
        unsigned kmin = std::min(newBound + 1, tree->nof_input);

        // Each new var in a parent node must have enough literals under it to make up for its representation in the unary number ;
        if (tree->lazyVars.size() >= kmin) // In most cases, only continue if the node has been affected by k-simplification
            return;                        // and its nof_input is smaller than the number of vars, leaving room for increase.

        increase   (tree->left, newBound);
        increase   (tree->right, newBound);
        increase_ua( tree->lazyVars, tree->left->lazyVars, tree->right->lazyVars, kmin);
    }

    void increase_ua( std::vector< std::shared_ptr<LazyVariable<T>> >& ogVars, std::vector< std::shared_ptr<LazyVariable<T>> >& aVars, std::vector< std::shared_ptr<LazyVariable<T>> >& bVars, unsigned rhs) {
        unsigned last = ogVars.size();

        for (unsigned i = last; i < rhs; ++i)
            ogVars.push_back( std::make_shared<LazyVariable<T>>(solver) );

        unsigned maxj = std::min(rhs, (unsigned)bVars.size());
        for (unsigned j = last; j < maxj; ++j) {
            ogVars[j]->addImpliquant({bVars[j]});
        }

        unsigned maxi = std::min(rhs, (unsigned)aVars.size());
        for (unsigned i = last; i < maxi; ++i) {
            ogVars[i]->addImpliquant({aVars[i]});
        }

        for (unsigned i = 1; i <= maxi; ++i) {
            unsigned maxj = std::min(rhs - i, (unsigned)bVars.size());
            unsigned minj = std::max((int)last - (int)i + 1, 1);
            for (unsigned j = minj; j <= maxj; ++j) {
                ogVars[ i + j - 1]->addImpliquant({aVars[ i - 1], bVars[ j - 1]});
            }
        }
    }


    void new_ua( std::vector< std::shared_ptr<LazyVariable<T>> >& ogVars, unsigned rhs, std::vector< std::shared_ptr<LazyVariable<T>> >& aVars, std::vector< std::shared_ptr<LazyVariable<T>> >& bVars) {
        // Creates a direct correspondance between an ogVar and a bVar of the same index
        unsigned kmin = std::min(rhs, (unsigned)bVars.size());
        for (unsigned j = 0; j < kmin; ++j) {
            ogVars[j]->addImpliquant( {bVars[j]});
        }

        // Same as above ; if aVar[index] is true, then ogVar[index] must be true as well
        kmin = std::min(rhs, (unsigned)aVars.size());
        for (unsigned i = 0; i < kmin; ++i) {
            ogVars[i]->addImpliquant( {aVars[i]});
        }

        // Handles the addition cases. Per example, if aVar[0] is true and bVar[2] is true, then ogVar[3] must be true.
        // Refer to a Totalizer Encoding tree.
        for (unsigned i = 1; i <= kmin; ++i) {
            unsigned minj = std::min(rhs - i, (unsigned)bVars.size());
            for (unsigned j = 1; j <= minj; ++j) {
                ogVars[ i + j - 1]->addImpliquant( {aVars[ i - 1], bVars[ j - 1]});
            }
        }
    }


};



