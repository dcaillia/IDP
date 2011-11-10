#include "TheoryMutatingVisitor.hpp"

#include "vocabulary.hpp"
#include "theory.hpp"
#include "term.hpp"
#include "error.hpp"

using namespace std;

Theory* TheoryMutatingVisitor::visit(Theory* t) {
	for (auto it = t->sentences().begin(); it != t->sentences().end(); ++it) {
		*it = (*it)->accept(this);
	}
	for (auto it = t->definitions().begin(); it != t->definitions().end(); ++it) {
		*it = (*it)->accept(this);
	}
	for (auto it = t->fixpdefs().begin(); it != t->fixpdefs().end(); ++it) {
		*it = (*it)->accept(this);
	}
	return t;
}

GroundTheory<GroundPolicy>* TheoryMutatingVisitor::visit(GroundTheory<GroundPolicy>* t) {
	// TODO
	return t;
}

GroundTheory<SolverPolicy>* TheoryMutatingVisitor::visit(GroundTheory<SolverPolicy>* t) {
	// TODO
	return t;
}

GroundTheory<PrintGroundPolicy>* TheoryMutatingVisitor::visit(GroundTheory<PrintGroundPolicy>* t) {
	// TODO
	return t;
}

Formula* TheoryMutatingVisitor::traverse(Formula* f) {
	for (size_t n = 0; n < f->subterms().size(); ++n) {
		f->subterm(n, f->subterms()[n]->accept(this));
	}
	for (size_t n = 0; n < f->subformulas().size(); ++n) {
		f->subformula(n, f->subformulas()[n]->accept(this));
	}
	return f;
}

Formula* TheoryMutatingVisitor::visit(PredForm* pf) {
	return traverse(pf);
}
Formula* TheoryMutatingVisitor::visit(EqChainForm* ef) {
	return traverse(ef);
}
Formula* TheoryMutatingVisitor::visit(EquivForm* ef) {
	return traverse(ef);
}
Formula* TheoryMutatingVisitor::visit(BoolForm* bf) {
	return traverse(bf);
}
Formula* TheoryMutatingVisitor::visit(QuantForm* qf) {
	return traverse(qf);
}
Formula* TheoryMutatingVisitor::visit(AggForm* af) {
	return traverse(af);
}

Rule* TheoryMutatingVisitor::visit(Rule* r) {
	r->body(r->body()->accept(this));
	return r;
}

Definition* TheoryMutatingVisitor::visit(Definition* d) {
	for (size_t n = 0; n < d->rules().size(); ++n) {
		d->rule(n, d->rules()[n]->accept(this));
	}
	return d;
}

FixpDef* TheoryMutatingVisitor::visit(FixpDef* fd) {
	for (size_t n = 0; n < fd->rules().size(); ++n) {
		fd->rule(n, fd->rules()[n]->accept(this));
	}
	for (size_t n = 0; n < fd->defs().size(); ++n) {
		fd->def(n, fd->defs()[n]->accept(this));
	}
	return fd;
}

Term* TheoryMutatingVisitor::traverse(Term* t) {
	for (size_t n = 0; n < t->subterms().size(); ++n) {
		t->subterm(n, t->subterms()[n]->accept(this));
	}
	for (size_t n = 0; n < t->subsets().size(); ++n) {
		t->subset(n, t->subsets()[n]->accept(this));
	}
	return t;
}

Term* TheoryMutatingVisitor::visit(VarTerm* vt) {
	return traverse(vt);
}
Term* TheoryMutatingVisitor::visit(FuncTerm* ft) {
	return traverse(ft);
}
Term* TheoryMutatingVisitor::visit(DomainTerm* dt) {
	return traverse(dt);
}
Term* TheoryMutatingVisitor::visit(AggTerm* at) {
	return traverse(at);
}

SetExpr* TheoryMutatingVisitor::traverse(SetExpr* s) {
	for (size_t n = 0; n < s->subterms().size(); ++n) {
		s->subterm(n, s->subterms()[n]->accept(this));
	}
	for (size_t n = 0; n < s->subformulas().size(); ++n) {
		s->subformula(n, s->subformulas()[n]->accept(this));
	}
	return s;
}

SetExpr* TheoryMutatingVisitor::visit(EnumSetExpr* es) {
	return traverse(es);
}
SetExpr* TheoryMutatingVisitor::visit(QuantSetExpr* qs) {
	return traverse(qs);
}