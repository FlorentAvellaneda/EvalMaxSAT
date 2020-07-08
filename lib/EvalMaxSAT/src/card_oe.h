#ifndef CARD_OE_H
#define CARD_OE_H

#include "virtualcard.h"
#include "lazyvariable.h"

#include "View.h"


#include <optional>
#include <cassert>
#include <memory>
#include <deque>



class Card_Lazy_OE : public VirtualCard {
    std::vector< std::shared_ptr<LazyVariable> > coutingVar;

    std::vector<int> clause;

public:

    virtual int atMost(unsigned int k) {
        if(k >= coutingVar.size()) {
            return 0;
        }

        return -coutingVar[k]->get();
    }

    virtual std::vector<int> getClause() {
        return clause;
    }

    Card_Lazy_OE(VirtualSAT * solver, const std::vector< int >& clause, std::optional<unsigned int> k = {})
        : VirtualCard (solver, clause), clause(clause) {

        k = k.value_or(clause.size()) + 1;

        if(*k > clause.size())
            *k = clause.size();

        std::vector< std::shared_ptr<LazyVariable> > lazyClause;

        for(auto lit: clause) {
            lazyClause.push_back( LazyVariable::encapsulate(lit) );
        }

        coutingVar = oe_4sel(lazyClause, *k);
    }

    virtual ~Card_Lazy_OE();

private:

    // The output is top k sorted and is a permutation of the inputs
    std::vector< std::shared_ptr<LazyVariable> > oe_4sel(const std::vector< std::shared_ptr<LazyVariable> > &input, unsigned int k);


    std::vector<std::shared_ptr<LazyVariable>> sort2(std::shared_ptr<LazyVariable> var1, std::shared_ptr<LazyVariable> var2) {
        std::vector<std::shared_ptr<LazyVariable>> result;

        result.push_back( LazyVariable::newVar(solver) );
        result.back()->addImpliquant({var1});
        result.back()->addImpliquant({var2});

        result.push_back( LazyVariable::newVar(solver) );
        result.back()->addImpliquant({var1, var2});

        return result;
    }

    std::vector<std::shared_ptr<LazyVariable>> sort2(const std::vector< std::shared_ptr<LazyVariable> > &input) {
        assert(input.size() == 2);
        return sort2(input[0], input[1]);
    }

    std::shared_ptr<LazyVariable> top(std::shared_ptr<LazyVariable> a, std::shared_ptr<LazyVariable> b);

    std::shared_ptr<LazyVariable> top(std::shared_ptr<LazyVariable> a, std::shared_ptr<LazyVariable> b, std::shared_ptr<LazyVariable> c);


    std::vector< std::shared_ptr<LazyVariable> > select(std::vector< std::shared_ptr<LazyVariable> > input, int k);

    std::vector< std::shared_ptr<LazyVariable> > oe_4merge(const MaLib::View< std::shared_ptr<LazyVariable> > &w, const MaLib::View< std::shared_ptr<LazyVariable> > &x, const MaLib::View< std::shared_ptr<LazyVariable> > &y, const MaLib::View< std::shared_ptr<LazyVariable> > &z, int k) {
        assert(k >= w.size());
        assert(w.size() >= x.size());
        assert(x.size() >= y.size());
        assert(y.size() >= z.size());
        assert(k <= w.size() + x.size() + y.size() + z.size());

        if(x.size() == 0) {
            return w.toVector();
        }

        if(w.size() == 1) {
            std::vector< std::shared_ptr<LazyVariable> > tmp;
            w.toVector(tmp);
            x.toVector(tmp);
            y.toVector(tmp);
            z.toVector(tmp);

            return select(tmp, k);
        }

        int sb = w.size()/2 + x.size()/2 + y.size()/2 + z.size()/2;
        int sa = sb + w.size()%2 + x.size()%2 + y.size()%2 + z.size()%2;

        int ka = std::min(sa, k/2+2);
        int kb = std::min(sb, k/2);

        auto a = oe_4merge ( MaLib::View(w, 0, -1, 2), MaLib::View(x, 0, -1, 2), MaLib::View(y, 0, -1, 2), MaLib::View(z, 0, -1, 2), ka );
        auto b = oe_4merge ( MaLib::View(w, 1, -1, 2), MaLib::View(x, 1, -1, 2), MaLib::View(y, 1, -1, 2), MaLib::View(z, 1, -1, 2), kb );

        auto result = oe_4combine( MaLib::View< std::shared_ptr<LazyVariable> >(a, 0, ka), MaLib::View< std::shared_ptr<LazyVariable> >(b, 0, kb), k );

        result.insert(result.end(), &a[ka], &a.back()+1);
        result.insert(result.end(), &b[kb], &b.back()+1);

        return result;
    }

    std::vector< std::shared_ptr<LazyVariable> > oe_4combine(const MaLib::View< std::shared_ptr<LazyVariable> > &x, const MaLib::View< std::shared_ptr<LazyVariable> > &y, int k);

};


#endif // CARD_OE_H
