#include "cardincremental.h"

#include "virtualsat.h"

CardIncremental::CardIncremental(VirtualSAT * solver, const std::vector<int>& clause, unsigned int bound)
    : VirtualCard (solver, clause, bound), _MAX(clause.size())
{
    std::deque<std::shared_ptr<TotTree>> nqueue;

    for (unsigned i = 0; i < _MAX; ++i) {
        std::shared_ptr<TotTree> tree = std::make_shared<TotTree>();

        tree->vars.resize(1);
        tree->vars[0]   = clause[i];
        tree->nof_input = 1;
        tree->left      = 0;
        tree->right     = 0;

        nqueue.push_back(tree);
    }

    while (nqueue.size() > 1) {
        auto l = nqueue.front();
        nqueue.pop_front();
        auto r = nqueue.front();
        nqueue.pop_front();

        auto node = std::make_shared<TotTree>();
        node->nof_input = l->nof_input + r->nof_input;
        node->left      = l;
        node->right     = r;

        unsigned kmin = std::min(bound + 1, node->nof_input);

        node->vars.resize(kmin);
        for (unsigned i = 0; i < kmin; ++i)
            node->vars[i] = newVar();

        new_ua(node->vars, kmin, l->vars, r->vars);
        nqueue.push_back(node);
    }

    _tree = nqueue.front();
}

void CardIncremental::add(const std::vector<int>& clause) {
    CardIncremental tb(solver, clause, _tree->vars.size()-1);

    unsigned n    = _tree->nof_input + tb._tree->nof_input;
    unsigned kmin = n;
    if(_tree->vars.size() < n)
        kmin = _tree->vars.size();


    std::shared_ptr<TotTree> tree = std::make_shared<TotTree>();
    tree->nof_input = n;
    tree->left      = _tree;
    tree->right     = tb._tree;

    tree->vars.resize(kmin);
    for (unsigned i = 0; i < kmin; ++i)
        tree->vars[i] = newVar();

    new_ua(tree->vars, kmin, _tree->vars, tb._tree->vars);

    _MAX += clause.size();
    _tree = tree;
}


void CardIncremental::increase_ua(std::vector<int>& ov, std::vector<int>& av, std::vector<int>& bv, unsigned rhs)
{
    unsigned last = ov.size();

    for (unsigned i = last; i < rhs; ++i)
        ov.push_back(newVar());

    // add the constraints
    // i = 0
    unsigned maxj = std::min(rhs, (unsigned)bv.size());
    for (unsigned j = last; j < maxj; ++j)
        addClause({-bv[j], ov[j]});

    // j = 0
    unsigned maxi = std::min(rhs, (unsigned)av.size());
    for (unsigned i = last; i < maxi; ++i)
        addClause({-av[i], ov[i]});

    // i, j > 0
    for (unsigned i = 1; i <= maxi; ++i) {
        unsigned maxj = std::min(rhs - i, (unsigned)bv.size());
        unsigned minj = std::max((int)last - (int)i + 1, 1);
        for (unsigned j = minj; j <= maxj; ++j)
            addClause({-av[i - 1], -bv[j - 1], ov[i + j - 1]});
    }
}


void CardIncremental::new_ua(std::vector<int>& ov, unsigned rhs, std::vector<int>& av, std::vector<int>& bv)
{
    // i = 0
    unsigned kmin = std::min(rhs, (unsigned)bv.size());
    for (unsigned j = 0; j < kmin; ++j) {
        addClause({-bv[j], ov[j]});
    }

    // j = 0
    kmin = std::min(rhs, (unsigned)av.size());
    for (unsigned i = 0; i < kmin; ++i) {
        addClause({-av[i], ov[i]});
    }

    // i, j > 0
    for (unsigned i = 1; i <= kmin; ++i) {
        unsigned minj = std::min(rhs - i, (unsigned)bv.size());
        for (unsigned j = 1; j <= minj; ++j) {
            addClause({-av[i - 1], -bv[j - 1], ov[i + j - 1]});
        }
    }
}






























CardIncremental_Lazy::CardIncremental_Lazy(VirtualSAT * solver, const std::vector<int>& clause, unsigned int bound)
    : VirtualCard (solver, clause, bound), _MAX(clause.size())
{
    std::deque<std::shared_ptr<TotTree>> nqueue;

    for (unsigned i = 0; i < _MAX; ++i) {
        std::shared_ptr<TotTree> tree = std::make_shared<TotTree>();

        tree->vars.resize(1);
        tree->vars[0]   = LazyVariable::encapsulate(clause[i]);
        tree->nof_input = 1;
        tree->left      = 0;
        tree->right     = 0;

        nqueue.push_back(tree);
    }

    while (nqueue.size() > 1) {
        auto l = nqueue.front();
        nqueue.pop_front();
        auto r = nqueue.front();
        nqueue.pop_front();

        auto node = std::make_shared<TotTree>();
        node->nof_input = l->nof_input + r->nof_input;
        node->left      = l;
        node->right     = r;

        unsigned kmin = std::min(bound + 1, node->nof_input);

        node->vars.resize(kmin);
        for (unsigned i = 0; i < kmin; ++i)
            node->vars[i] = LazyVariable::newVar(solver); //newVar();

        new_ua(node->vars, kmin, l->vars, r->vars);
        nqueue.push_back(node);
    }

    _tree = nqueue.front();
}

void CardIncremental_Lazy::add(const std::vector<int>& clause) {
    CardIncremental_Lazy tb(solver, clause, _tree->vars.size()-1);

    unsigned n    = _tree->nof_input + tb._tree->nof_input;
    unsigned kmin = n;
    if(_tree->vars.size() < n)
        kmin = _tree->vars.size();


    std::shared_ptr<TotTree> tree = std::make_shared<TotTree>();
    tree->nof_input = n;
    tree->left      = _tree;
    tree->right     = tb._tree;

    tree->vars.resize(kmin);
    for (unsigned i = 0; i < kmin; ++i)
        tree->vars[i] = LazyVariable::newVar(solver);

    new_ua(tree->vars, kmin, _tree->vars, tb._tree->vars);

    _MAX += clause.size();
    _tree = tree;
}


void CardIncremental_Lazy::increase_ua(std::vector< std::shared_ptr<LazyVariable> >& ov, std::vector< std::shared_ptr<LazyVariable> >& av, std::vector< std::shared_ptr<LazyVariable> >& bv, unsigned rhs)
{
    unsigned last = ov.size();

    for (unsigned i = last; i < rhs; ++i)
        ov.push_back( LazyVariable::newVar(solver) );

    // add the constraints
    // i = 0
    unsigned maxj = std::min(rhs, (unsigned)bv.size());
    for (unsigned j = last; j < maxj; ++j) {
        //addClause({-bv[j], ov[j]});
        ov[j]->addImpliquant({bv[j]});
    }

    // j = 0
    unsigned maxi = std::min(rhs, (unsigned)av.size());
    for (unsigned i = last; i < maxi; ++i) {
        //addClause({-av[i], ov[i]});
        ov[i]->addImpliquant({av[i]});
    }

    // i, j > 0
    for (unsigned i = 1; i <= maxi; ++i) {
        unsigned maxj = std::min(rhs - i, (unsigned)bv.size());
        unsigned minj = std::max((int)last - (int)i + 1, 1);
        for (unsigned j = minj; j <= maxj; ++j) {
            //addClause({-av[i - 1], -bv[j - 1], ov[i + j - 1]});
            ov[i + j - 1]->addImpliquant({av[i - 1], bv[j - 1]});
        }
    }
}


void CardIncremental_Lazy::new_ua(std::vector< std::shared_ptr<LazyVariable> >& ov, unsigned rhs, std::vector< std::shared_ptr<LazyVariable> >& av, std::vector< std::shared_ptr<LazyVariable> >& bv)
{
    // i = 0
    unsigned kmin = std::min(rhs, (unsigned)bv.size());
    for (unsigned j = 0; j < kmin; ++j) {
        //addClause({-bv[j], ov[j]});
        ov[j]->addImpliquant({bv[j]});
    }

    // j = 0
    kmin = std::min(rhs, (unsigned)av.size());
    for (unsigned i = 0; i < kmin; ++i) {
        //addClause({-av[i], ov[i]});
        ov[i]->addImpliquant({av[i]});
    }

    // i, j > 0
    for (unsigned i = 1; i <= kmin; ++i) {
        unsigned minj = std::min(rhs - i, (unsigned)bv.size());
        for (unsigned j = 1; j <= minj; ++j) {
            //addClause({-av[i - 1], -bv[j - 1], ov[i + j - 1]});
            ov[i + j - 1]->addImpliquant({av[i - 1], bv[j - 1]});
        }
    }
}

