#include "card_oe.h"

#include "virtualsat.h"

// The output is top k sorted and is a permutation of the inputs
std::vector< std::shared_ptr<LazyVariable> > Card_Lazy_OE::oe_4sel(const std::vector< std::shared_ptr<LazyVariable> > &input, unsigned int k) {

    assert(k <= input.size());

    if(k==0)
        return {};
    if(input.size() == 1)
        return input;

    if(k==1) {
        auto v = solver->newLazyVariable();
        for(unsigned int i=0; i<input.size(); i++) {
            v->addImpliquant({input[i]});
        }

        return {v};
    }

    unsigned int n[4];

    if((input.size() < 8) || (k == input.size())) {
        n[1] = static_cast<int>((input.size()+2)/4);
        n[2] = static_cast<int>((input.size()+1)/4);
        n[3] = static_cast<int>(input.size()/4);
    } else {
        int v = 1;
        while(v <= k/6) {
            v = v << 1;
        }

        if(v <= input.size()/4) {
            n[1] = n[2] = n[3] = v;
        } else {
            n[1] = n[2] = n[3] = k/4;
        }
    }

    n[0] = input.size() - n[1] - n[2] - n[3];
    unsigned int offset = 0;

    unsigned int tabK[4];
    std::vector<std::vector< std::shared_ptr<LazyVariable> >> y;
    unsigned int s=0;
    for(unsigned int i=0; i<4; i++) {
        tabK[i] = std::min(k, n[i]); //k<n[i]?k:n[i]; // min(k,n[i])

        y.push_back( oe_4sel( std::vector< std::shared_ptr<LazyVariable> >(&(input[offset]), &(input[offset+n[i]/*-1*/])) , tabK[i]) );  // Optimisation possible, ne pas copier, mais faire par reference
        offset += n[i];
        s += tabK[i];
    }



    std::vector< std::shared_ptr<LazyVariable> > result = oe_4merge(
                View(y[0], 0, tabK[0], 1),
                View(y[1], 0, tabK[1], 1),
                View(y[2], 0, tabK[2], 1),
                View(y[3], 0, tabK[3], 1),
                k );

/*
    std::vector< std::shared_ptr<LazyVariable> > result = oe_4merge(
            std::vector< std::shared_ptr<LazyVariable> >(&y[0][0], &y[0][ tabK[0] ]),
            std::vector< std::shared_ptr<LazyVariable> >(&y[1][0], &y[1][ tabK[1] ]),
            std::vector< std::shared_ptr<LazyVariable> >(&y[2][0], &y[2][ tabK[2] ]),
            std::vector< std::shared_ptr<LazyVariable> >(&y[3][0], &y[3][ tabK[3] ]),
            k );
*/

    for(int i=0; i<4; i++) {
        result.insert(result.end(), &y[i][ tabK[i] ], &y[i].back()+1);
    }




    assert(result.size() == k);
    return result;
}


std::shared_ptr<LazyVariable> Card_Lazy_OE::top(std::shared_ptr<LazyVariable> a, std::shared_ptr<LazyVariable> b) {

    // a V b => result
    auto result = solver->newLazyVariable();
    //auto result = std::make_shared<LazyVariable>(solver);

    result->addImpliquant({a});
    result->addImpliquant({b});

    return result;
}


std::shared_ptr<LazyVariable> Card_Lazy_OE::top(std::shared_ptr<LazyVariable> a, std::shared_ptr<LazyVariable> b, std::shared_ptr<LazyVariable> c) {

    // a V b V c => res
    auto result = solver->newLazyVariable();

    result->addImpliquant({a});
    result->addImpliquant({b});
    result->addImpliquant({c});

    return result;
}


std::vector< std::shared_ptr<LazyVariable> > Card_Lazy_OE::oe_4combine(const MaLib::View<std::shared_ptr<LazyVariable> > &x, const MaLib::View<std::shared_ptr<LazyVariable> > &y, int k) {

    assert(y.size() <= k/2);
    assert(x.size() <= k/2 + 2);
    assert(k <= x.size() + y.size());

    std::vector< std::shared_ptr<LazyVariable> > result;

    for(int i=0; i<k; i++) {
        result.push_back( solver->newLazyVariable() );
    }

    auto Y = [&result](unsigned int id){
        if(result.size() > id*2+1)
            return result[id*2+1];
        return std::shared_ptr<LazyVariable>(nullptr);
    };

    auto X = [&result](unsigned int id){
        if(id*2 < result.size())
            return result[id*2];
        return std::shared_ptr<LazyVariable>(nullptr);
    };

    // y[i] => y''[i]
    for(unsigned int i=0; i<y.size(); i++) {
        if(Y(i)) {
            //std::cout << "y["<<i<<"] => Y("<<i<<")" << std::endl;
            Y(i)->addImpliquant({y[i]});
        }
    }

    // x[i+2] => y''[i]
    for(unsigned int i=0; i<x.size()-2; i++) {
        if(Y(i)) {
            //std::cout << "x["<<i+2<<"] => Y("<<i<<")" << std::endl;
            Y(i)->addImpliquant({x[i+2]});
        }
    }

    // y[i−1] & x[i+1] => y''[i]    <===>   -y[i−1]  V  -x[i+1]  V  y''[i]
    {
        if(x.size() > 1) {
            // Cas particulier quand i = 0
            if(Y(0)) {
                //std::cout << "x[1] => Y(0)" << std::endl;
                Y(0)->addImpliquant({x[1]});
            }

            int max = std::min(static_cast<int>(y.size()+1), static_cast<int>(x.size()-1));
            for(int i=1; i<max; i++) {
                if(Y(i)) {
                    //std::cout << "y["<<i-1<<"] ^ x["<<i+1<<"] => Y["<<i<<"]"<<std::endl;
                    Y(i)->addImpliquant({y[i-1], x[i+1]});
                }
            }
        }
    }

    // y[i−1] & x[i] => x''[i]      <===>   -y[i−1]  V  -x[i]  V  x''[i]
    {
        // Cas particulier quand i=0
        if(X(0)) {
            //std::cout << "x[0] => X(0)" << std::endl;
            X(0)->addImpliquant({x[0]});
        }
        int max = std::min(static_cast<int>(x.size()), static_cast<int>(y.size()+1));
        for(int i=1; i<max; i++) {
            if(X(i)) {
                //std::cout << "y["<<i-1<<"] ^ x["<<i<<"] => X("<<i<<")"<<std::endl;
                X(i)->addImpliquant({y[i-1], x[i]});
            }
        }
    }

    // y[i−2] & x[i+1] => x''[i]    <===>   -y[i−2]  V  -x[i+1]  V  x''[i]
    {
        // Cas particulier quand i={0,1}
        //addClause(-x[1], X(0)); inutile car x[0] => X(0)
        if(x.size() > 2) {
            if(X(1)) {
                //std::cout << "x[2] => X(1)" << std::endl;
                X(1)->addImpliquant({x[2]});
            }
            int max = std::min(static_cast<int>(x.size()-1), static_cast<int>(y.size()+2));
            for(int i=2; i<max; i++) {
                if(X(i)) {
                    //std::cout << "y["<<i-2<<"] ^ x["<<i+1<<"] => X("<<i<<")" << std::endl;
                    X(i)->addImpliquant({y[i-2], x[i+1]});
                }
            }
        }
    }

    //LOG << "return " << result << std::endl;
    return result;
}


std::vector< std::shared_ptr<LazyVariable> > Card_Lazy_OE::select(std::vector< std::shared_ptr<LazyVariable> > input, int k) {
    assert(k <= input.size());

    switch (k) {
        case 0: {
            assert(false);
            break;
        }

        case 1: {
            auto res = solver->newLazyVariable();
            for(auto lit: input) {
                res->addImpliquant({lit});
            }
            return {res};
        }

        case 2: {
            if(input.size() == 2) {
                return sort2(input);
            }

            if(input.size() == 3) {
                auto a = sort2(input[0], input[1]);
                auto b = sort2(a[0], input[2]);
                auto c = top(a[1], b[1]);

                return {b[0], c};
            }

            if(input.size() == 4) {
                auto a = sort2(input[0], input[1]);
                auto b = sort2(input[2], input[3]);

                auto odd = sort2(a[0], b[0]);

                return {odd[0], top(a[1], b[1], odd[1])};
            }
            assert(false);
            break;
        }

        case 3: {
            if(input.size() == 3) {
                auto a = sort2(input[0], input[1]);
                auto b = sort2(a[0], input[2]);
                auto c = sort2(a[1], b[1]);

                return {b[0], c[0], c[1]};
            }

            if(input.size() == 4) {
                auto a = sort2(input[0], input[1]);
                auto b = sort2(input[2], input[3]);

                auto odd = sort2(a[0], b[0]);
                auto even = top(a[1], b[1]);

                auto center = sort2( even, odd[1] );

                return {odd[0], center[0], center[1]};
            }
            assert(false);
            break;
        }

        case 4: {
            auto a = sort2(input[0], input[1]);
            auto b = sort2(input[2], input[3]);

            auto odd = sort2(a[0], b[0]);
            auto even = sort2(a[1], b[1]);

            auto center = sort2( even[0], odd[1] );

            return {odd[0], center[0], center[1], even[1]};
        }

        default: {
            assert(false);
        }
    }
    assert(false);

}

/*
Card_OE::~Card_OE() {

}
*/

Card_Lazy_OE::~Card_Lazy_OE() {

}






