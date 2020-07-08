#ifndef CARDINCREMENTAL_H
#define CARDINCREMENTAL_H

#include "virtualcard.h"
#include <optional>
#include <cassert>
#include <memory>
#include <deque>
#include <iostream>

#include "lazyvariable.h"


class CardIncremental : public VirtualCard {

    typedef struct TotTree {
        std::vector<int> vars;
        unsigned nof_input;
        std::shared_ptr<TotTree> left;
        std::shared_ptr<TotTree> right;

        void print(std::ostream& os, bool first=true) const {
            if(left == nullptr && right == nullptr) {
                assert(vars.size() == 1);
                if(!first)
                    os << ", ";
                os << vars[0];
            } else {
                if(left != nullptr) {
                    left->print(os, first);
                }
                if(right != nullptr) {
                    right->print(os, false);
                }
            }
        }

        void getClause_rec(std::vector<int> &result) {
            if(left == nullptr && right == nullptr) {
                assert(vars.size() == 1);
                result.push_back(vars[0]);
            } else {
                if(left != nullptr) {
                    left->getClause_rec(result);
                }
                if(right != nullptr) {
                    right->getClause_rec(result);
                }
            }
        }
    } TotTree;

    std::shared_ptr<TotTree> _tree;
    unsigned int _MAX;
public:

    void print(std::ostream& os) const override {
        os << "[";
        _tree->print(os, true);
        os << "]";
    }

    unsigned int size() const override {
        return _MAX;
    }
/*
    virtual std::vector<int> getClause() {
        std::vector<int> result;
        _tree->getClause_rec(result);
        return result;
    }
*/
    void add(const std::vector<int>& clause);


    virtual int atMost(unsigned int k) override {
        if( k >= _MAX ) {
            return 0;
        }

        if( k >= _tree->vars.size() ) {
            increase(k);
        }
        assert(k < _tree->vars.size());

        return -_tree->vars[k];
    }

    CardIncremental(VirtualSAT * solver, const std::vector<int>& clause, unsigned int bound=1);

    void increase(unsigned int newBound) {
        increase(_tree, newBound);
    }

private:

    void increase(std::shared_ptr<TotTree> tree, unsigned newBound)
    {
        unsigned kmin = std::min(newBound + 1, tree->nof_input);

        if (kmin <= tree->vars.size())
            return;

        increase   (tree->left, newBound);
        increase   (tree->right, newBound);
        increase_ua(tree->vars, tree->left->vars, tree->right->vars, kmin);
    }

    void increase_ua(std::vector<int>& ov, std::vector<int>& av, std::vector<int>& bv, unsigned rhs);


    void new_ua(std::vector<int>& ov, unsigned rhs, std::vector<int>& av, std::vector<int>& bv);


    friend class VirtualSAT;
};


class CardIncremental_Lazy : public VirtualCard {

    typedef struct TotTree {
        std::vector< std::shared_ptr<LazyVariable> > vars;
        unsigned nof_input;
        std::shared_ptr<TotTree> left;
        std::shared_ptr<TotTree> right;

        void print(std::ostream& os, bool first=true) const {
            if(left == nullptr && right == nullptr) {
                assert(vars.size() == 1);
                if(!first)
                    os << ", ";
                os << vars[0];
            } else {
                if(left != nullptr) {
                    left->print(os, first);
                }
                if(right != nullptr) {
                    right->print(os, false);
                }
            }
        }
    } TotTree;

    std::shared_ptr<TotTree> _tree;
    unsigned int _MAX;
public:

    void print(std::ostream& os) const override {
        os << "[";
        _tree->print(os, true);
        os << "]";
    }

    unsigned int size() const override {
        return _MAX;
    }

    void add(const std::vector<int>& clause);


    virtual int atMost(unsigned int k) override {
        if( k >= _MAX ) {
            return 0;
        }

        if( k >= _tree->vars.size() ) {
            increase(k);
        }
        assert(k < _tree->vars.size());

        return -_tree->vars[k]->get();
    }

    CardIncremental_Lazy(VirtualSAT * solver, const std::vector<int>& clause, unsigned int bound=1);

    void increase(unsigned int newBound) {
        increase(_tree, newBound);
    }

private:

    void increase(std::shared_ptr<TotTree> tree, unsigned newBound)
    {
        unsigned kmin = std::min(newBound + 1, tree->nof_input);

        if (kmin <= tree->vars.size())
            return;

        increase   (tree->left, newBound);
        increase   (tree->right, newBound);
        increase_ua(tree->vars, tree->left->vars, tree->right->vars, kmin);
    }

    void increase_ua(std::vector< std::shared_ptr<LazyVariable> >& ov, std::vector< std::shared_ptr<LazyVariable> >& av, std::vector< std::shared_ptr<LazyVariable> >& bv, unsigned rhs);


    void new_ua(std::vector< std::shared_ptr<LazyVariable> >& ov, unsigned rhs, std::vector< std::shared_ptr<LazyVariable> >& av, std::vector< std::shared_ptr<LazyVariable> >& bv);


    friend class VirtualSAT;
};


#endif // CARDINCREMENTAL_H
