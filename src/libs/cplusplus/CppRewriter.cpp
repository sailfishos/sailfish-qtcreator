/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "CppRewriter.h"

#include "Overview.h"

#include <cplusplus/TypeVisitor.h>
#include <cplusplus/NameVisitor.h>
#include <cplusplus/CoreTypes.h>
#include <cplusplus/Symbols.h>
#include <cplusplus/Literals.h>
#include <cplusplus/Names.h>
#include <cplusplus/Scope.h>

#include <QVarLengthArray>
#include <QDebug>

namespace CPlusPlus {

class Rewrite
{
public:
    Rewrite(Control *control, SubstitutionEnvironment *env)
        : control(control), env(env), rewriteType(this), rewriteName(this) {}

    class RewriteType: public TypeVisitor
    {
        Rewrite *rewrite;
        QList<FullySpecifiedType> temps;

        Control *control() const
        { return rewrite->control; }

        void accept(const FullySpecifiedType &ty)
        {
            TypeVisitor::accept(ty.type());
            unsigned flags = ty.flags();
            if (!temps.isEmpty()) {
                flags |= temps.back().flags();
                temps.back().setFlags(flags);
            }
        }

    public:
        RewriteType(Rewrite *r): rewrite(r) {}

        FullySpecifiedType operator()(const FullySpecifiedType &ty)
        {
            accept(ty);
            return (!temps.isEmpty()) ? temps.takeLast() : ty;
        }

        void visit(UndefinedType *) override
        {
            temps.append(FullySpecifiedType());
        }

        void visit(VoidType *) override
        {
            temps.append(control()->voidType());
        }

        void visit(IntegerType *type) override
        {
            temps.append(control()->integerType(type->kind()));
        }

        void visit(FloatType *type) override
        {
            temps.append(control()->floatType(type->kind()));
        }

        void visit(PointerToMemberType *type) override
        {
            const Name *memberName = rewrite->rewriteName(type->memberName());
            const FullySpecifiedType elementType = rewrite->rewriteType(type->elementType());
            temps.append(control()->pointerToMemberType(memberName, elementType));
        }

        void visit(PointerType *type) override
        {
            const FullySpecifiedType elementType = rewrite->rewriteType(type->elementType());
            temps.append(control()->pointerType(elementType));
        }

        void visit(ReferenceType *type) override
        {
            const FullySpecifiedType elementType = rewrite->rewriteType(type->elementType());
            temps.append(control()->referenceType(elementType, type->isRvalueReference()));
        }

        void visit(ArrayType *type) override
        {
            const FullySpecifiedType elementType = rewrite->rewriteType(type->elementType());
            temps.append(control()->arrayType(elementType, type->size()));
        }

        void visit(NamedType *type) override
        {
            FullySpecifiedType ty = rewrite->env->apply(type->name(), rewrite);
            if (! ty->isUndefinedType()) {
                temps.append(ty);
            } else {
                const Name *name = rewrite->rewriteName(type->name());
                temps.append(control()->namedType(name));
            }
        }

        void visit(Function *type) override
        {
            Function *funTy = control()->newFunction(0, nullptr);
            funTy->copy(type);
            funTy->setConst(type->isConst());
            funTy->setVolatile(type->isVolatile());
            funTy->setRefQualifier(type->refQualifier());
            funTy->setExceptionSpecification(type->exceptionSpecification());

            funTy->setName(rewrite->rewriteName(type->name()));

            funTy->setReturnType(rewrite->rewriteType(type->returnType()));

            // Function parameters have the function's enclosing scope.
            Scope *scope = nullptr;
            ClassOrNamespace *target = nullptr;
            if (rewrite->env->context().bindings())
                target = rewrite->env->context().lookupType(type->enclosingScope());
            UseMinimalNames useMinimalNames(target);
            if (target) {
                scope = rewrite->env->switchScope(type->enclosingScope());
                rewrite->env->enter(&useMinimalNames);
            }

            for (unsigned i = 0, argc = type->argumentCount(); i < argc; ++i) {
                Symbol *arg = type->argumentAt(i);

                Argument *newArg = control()->newArgument(0, nullptr);
                newArg->copy(arg);
                newArg->setName(rewrite->rewriteName(arg->name()));
                newArg->setType(rewrite->rewriteType(arg->type()));

                // the copy() call above set the scope to 'type'
                // reset it to 0 before adding addMember to avoid assert
                newArg->resetEnclosingScope();
                funTy->addMember(newArg);
            }

            if (target) {
                rewrite->env->switchScope(scope);
                rewrite->env->leave();
            }

            temps.append(funTy);
        }

        void visit(Namespace *type) override
        {
            qWarning() << Q_FUNC_INFO;
            temps.append(type);
        }

        void visit(Class *type) override
        {
            qWarning() << Q_FUNC_INFO;
            temps.append(type);
        }

        void visit(Enum *type) override
        {
            qWarning() << Q_FUNC_INFO;
            temps.append(type);
        }

        void visit(ForwardClassDeclaration *type) override
        {
            qWarning() << Q_FUNC_INFO;
            temps.append(type);
        }

        void visit(ObjCClass *type) override
        {
            qWarning() << Q_FUNC_INFO;
            temps.append(type);
        }

        void visit(ObjCProtocol *type) override
        {
            qWarning() << Q_FUNC_INFO;
            temps.append(type);
        }

        void visit(ObjCMethod *type) override
        {
            qWarning() << Q_FUNC_INFO;
            temps.append(type);
        }

        void visit(ObjCForwardClassDeclaration *type) override
        {
            qWarning() << Q_FUNC_INFO;
            temps.append(type);
        }

        void visit(ObjCForwardProtocolDeclaration *type) override
        {
            qWarning() << Q_FUNC_INFO;
            temps.append(type);
        }

    };

    class RewriteName: public NameVisitor
    {
        Rewrite *rewrite;
        QList<const Name *> temps;

        Control *control() const
        { return rewrite->control; }

        const Identifier *identifier(const Identifier *other) const
        {
            if (! other)
                return nullptr;

            return control()->identifier(other->chars(), other->size());
        }

    public:
        RewriteName(Rewrite *r): rewrite(r) {}

        const Name *operator()(const Name *name)
        {
            if (! name)
                return nullptr;

            accept(name);
            return (!temps.isEmpty()) ? temps.takeLast() : name;
        }

        void visit(const QualifiedNameId *name) override
        {
            const Name *base = rewrite->rewriteName(name->base());
            const Name *n = rewrite->rewriteName(name->name());
            temps.append(control()->qualifiedNameId(base, n));
        }

        void visit(const Identifier *name) override
        {
            temps.append(control()->identifier(name->chars(), name->size()));
        }

        void visit(const TemplateNameId *name) override
        {
            QVarLengthArray<TemplateArgument, 8> args(name->templateArgumentCount());
            for (int i = 0; i < name->templateArgumentCount(); ++i)
                args[i] = rewrite->rewriteType(name->templateArgumentAt(i).type());
            temps.append(control()->templateNameId(identifier(name->identifier()), name->isSpecialization(),
                                                   args.data(), args.size()));
        }

        void visit(const DestructorNameId *name) override
        {
            temps.append(control()->destructorNameId(identifier(name->identifier())));
        }

        void visit(const OperatorNameId *name) override
        {
            temps.append(control()->operatorNameId(name->kind()));
        }

        void visit(const ConversionNameId *name) override
        {
            FullySpecifiedType ty = rewrite->rewriteType(name->type());
            temps.append(control()->conversionNameId(ty));
        }

        void visit(const SelectorNameId *name) override
        {
            QVarLengthArray<const Name *, 8> names(name->nameCount());
            for (int i = 0; i < name->nameCount(); ++i)
                names[i] = rewrite->rewriteName(name->nameAt(i));
            temps.append(control()->selectorNameId(names.constData(), names.size(), name->hasArguments()));
        }
    };

public: // attributes
    Control *control;
    SubstitutionEnvironment *env;
    RewriteType rewriteType;
    RewriteName rewriteName;
};

SubstitutionEnvironment::SubstitutionEnvironment()
    : _scope(nullptr)
{
}

FullySpecifiedType SubstitutionEnvironment::apply(const Name *name, Rewrite *rewrite) const
{
    if (name) {
        for (int index = _substs.size() - 1; index != -1; --index) {
            const Substitution *subst = _substs.at(index);

            FullySpecifiedType ty = subst->apply(name, rewrite);
            if (! ty->isUndefinedType())
                return ty;
        }
    }

    return FullySpecifiedType();
}

void SubstitutionEnvironment::enter(Substitution *subst)
{
    _substs.append(subst);
}

void SubstitutionEnvironment::leave()
{
    _substs.removeLast();
}

Scope *SubstitutionEnvironment::scope() const
{
    return _scope;
}

Scope *SubstitutionEnvironment::switchScope(Scope *scope)
{
    Scope *previous = _scope;
    _scope = scope;
    return previous;
}

const LookupContext &SubstitutionEnvironment::context() const
{
    return _context;
}

void SubstitutionEnvironment::setContext(const LookupContext &context)
{
    _context = context;
}

SubstitutionMap::SubstitutionMap()
{

}

SubstitutionMap::~SubstitutionMap()
{

}

void SubstitutionMap::bind(const Name *name, const FullySpecifiedType &ty)
{
    _map.append(qMakePair(name, ty));
}

FullySpecifiedType SubstitutionMap::apply(const Name *name, Rewrite *) const
{
    for (int n = _map.size() - 1; n != -1; --n) {
        const QPair<const Name *, FullySpecifiedType> &p = _map.at(n);

        if (name->match(p.first))
            return p.second;
    }

    return FullySpecifiedType();
}


UseMinimalNames::UseMinimalNames(ClassOrNamespace *target)
    : _target(target)
{

}

UseMinimalNames::~UseMinimalNames()
{

}

FullySpecifiedType UseMinimalNames::apply(const Name *name, Rewrite *rewrite) const
{
    SubstitutionEnvironment *env = rewrite->env;
    Scope *scope = env->scope();

    if (name->isTemplateNameId() ||
            (name->isQualifiedNameId() && name->asQualifiedNameId()->name()->isTemplateNameId()))
        return FullySpecifiedType();

    if (! scope)
        return FullySpecifiedType();

    const LookupContext &context = env->context();
    Control *control = rewrite->control;

    const QList<LookupItem> results = context.lookup(name, scope);
    if (!results.isEmpty()) {
        const LookupItem &r = results.first();
        if (Symbol *d = r.declaration())
            return control->namedType(LookupContext::minimalName(d, _target, control));

        return r.type();
    }

    return FullySpecifiedType();
}


UseQualifiedNames::UseQualifiedNames()
    : UseMinimalNames(nullptr)
{

}

UseQualifiedNames::~UseQualifiedNames()
{

}


FullySpecifiedType rewriteType(const FullySpecifiedType &type,
                               SubstitutionEnvironment *env,
                               Control *control)
{
    Rewrite rewrite(control, env);
    return rewrite.rewriteType(type);
}

const Name *rewriteName(const Name *name,
                        SubstitutionEnvironment *env,
                        Control *control)
{
    Rewrite rewrite(control, env);
    return rewrite.rewriteName(name);
}

} // namespace CPlusPlus
