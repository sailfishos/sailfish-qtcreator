// Copyright (c) 2008 Roberto Raggi <roberto.raggi@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once

#include "CPlusPlusForwardDeclarations.h"
#include "Symbol.h"
#include "Type.h"
#include "FullySpecifiedType.h"
#include "Scope.h"
#include <vector>

namespace CPlusPlus {

class StringLiteral;

class CPLUSPLUS_EXPORT UsingNamespaceDirective: public Symbol
{
public:
    UsingNamespaceDirective(TranslationUnit *translationUnit, int sourceLocation, const Name *name);
    UsingNamespaceDirective(Clone *clone, Subst *subst, UsingNamespaceDirective *original);
    virtual ~UsingNamespaceDirective();

    // Symbol's interface
    FullySpecifiedType type() const override;

    const UsingNamespaceDirective *asUsingNamespaceDirective() const override
    { return this; }

    UsingNamespaceDirective *asUsingNamespaceDirective() override
    { return this; }

protected:
    void visitSymbol0(SymbolVisitor *visitor) override;
};

class CPLUSPLUS_EXPORT UsingDeclaration: public Symbol
{
public:
    UsingDeclaration(TranslationUnit *translationUnit, int sourceLocation, const Name *name);
    UsingDeclaration(Clone *clone, Subst *subst, UsingDeclaration *original);
    virtual ~UsingDeclaration();

    // Symbol's interface
    FullySpecifiedType type() const override;

    const UsingDeclaration *asUsingDeclaration() const override
    { return this; }

    UsingDeclaration *asUsingDeclaration() override
    { return this; }

protected:
    void visitSymbol0(SymbolVisitor *visitor) override;
};

class CPLUSPLUS_EXPORT NamespaceAlias: public Symbol
{
public:
    NamespaceAlias(TranslationUnit *translationUnit, int sourceLocation, const Name *name);
    NamespaceAlias(Clone *clone, Subst *subst, NamespaceAlias *original);
    virtual ~NamespaceAlias();

    const Name *namespaceName() const;
    void setNamespaceName(const Name *namespaceName);

    // Symbol's interface
    FullySpecifiedType type() const override;

    const NamespaceAlias *asNamespaceAlias() const override
    { return this; }

    NamespaceAlias *asNamespaceAlias() override
    { return this; }

protected:
    void visitSymbol0(SymbolVisitor *visitor) override;

private:
    const Name *_namespaceName;
};

class CPLUSPLUS_EXPORT Declaration: public Symbol
{
public:
    Declaration(TranslationUnit *translationUnit, int sourceLocation, const Name *name);
    Declaration(Clone *clone, Subst *subst, Declaration *original);
    virtual ~Declaration();

    void setType(const FullySpecifiedType &type);
    void setInitializer(StringLiteral const* initializer);

    // Symbol's interface
    FullySpecifiedType type() const override;
    const StringLiteral *getInitializer() const;

    const Declaration *asDeclaration() const override
    { return this; }

    Declaration *asDeclaration() override
    { return this; }

    virtual EnumeratorDeclaration *asEnumeratorDeclarator()
    { return nullptr; }

    virtual const EnumeratorDeclaration *asEnumeratorDeclarator() const
    { return nullptr; }

protected:
    void visitSymbol0(SymbolVisitor *visitor) override;

private:
    FullySpecifiedType _type;
    const StringLiteral *_initializer;
};

class CPLUSPLUS_EXPORT EnumeratorDeclaration: public Declaration
{
public:
    EnumeratorDeclaration(TranslationUnit *translationUnit, int sourceLocation, const Name *name);
    virtual ~EnumeratorDeclaration();

    const StringLiteral *constantValue() const;
    void setConstantValue(const StringLiteral *constantValue);

    EnumeratorDeclaration *asEnumeratorDeclarator() override
    { return this; }

    const EnumeratorDeclaration *asEnumeratorDeclarator() const override
    { return this; }

private:
    const StringLiteral *_constantValue;
};

class CPLUSPLUS_EXPORT Argument: public Symbol
{
public:
    Argument(TranslationUnit *translationUnit, int sourceLocation, const Name *name);
    Argument(Clone *clone, Subst *subst, Argument *original);
    virtual ~Argument();

    void setType(const FullySpecifiedType &type);

    bool hasInitializer() const;

    const StringLiteral *initializer() const;
    void setInitializer(const StringLiteral *initializer);

    // Symbol's interface
    FullySpecifiedType type() const override;

    const Argument *asArgument() const override
    { return this; }

    Argument *asArgument() override
    { return this; }

protected:
    void visitSymbol0(SymbolVisitor *visitor) override;

private:
    const StringLiteral *_initializer;
    FullySpecifiedType _type;
};

class CPLUSPLUS_EXPORT TypenameArgument: public Symbol
{
public:
    TypenameArgument(TranslationUnit *translationUnit, int sourceLocation, const Name *name);
    TypenameArgument(Clone *clone, Subst *subst, TypenameArgument *original);
    virtual ~TypenameArgument();

    void setType(const FullySpecifiedType &type);
    void setClassDeclarator(bool isClassDecl) { _isClassDeclarator = isClassDecl; }
    bool isClassDeclarator() const { return _isClassDeclarator; }

    // Symbol's interface
    FullySpecifiedType type() const override;

    const TypenameArgument *asTypenameArgument() const override
    { return this; }

    TypenameArgument *asTypenameArgument() override
    { return this; }

protected:
    void visitSymbol0(SymbolVisitor *visitor) override;

private:
    FullySpecifiedType _type;
    bool _isClassDeclarator;
};

class CPLUSPLUS_EXPORT Block: public Scope
{
public:
    Block(TranslationUnit *translationUnit, int sourceLocation);
    Block(Clone *clone, Subst *subst, Block *original);
    virtual ~Block();

    // Symbol's interface
    FullySpecifiedType type() const override;

    const Block *asBlock() const override
    { return this; }

    Block *asBlock() override
    { return this; }

protected:
    void visitSymbol0(SymbolVisitor *visitor) override;
};

class CPLUSPLUS_EXPORT ForwardClassDeclaration: public Symbol, public Type
{
public:
    ForwardClassDeclaration(TranslationUnit *translationUnit, int sourceLocation, const Name *name);
    ForwardClassDeclaration(Clone *clone, Subst *subst, ForwardClassDeclaration *original);
    virtual ~ForwardClassDeclaration();

    // Symbol's interface
    FullySpecifiedType type() const override;

    const ForwardClassDeclaration *asForwardClassDeclaration() const override
    { return this; }

    ForwardClassDeclaration *asForwardClassDeclaration() override
    { return this; }

    // Type's interface
    const ForwardClassDeclaration *asForwardClassDeclarationType() const override
    { return this; }

    ForwardClassDeclaration *asForwardClassDeclarationType() override
    { return this; }

protected:
    void visitSymbol0(SymbolVisitor *visitor) override;
    void accept0(TypeVisitor *visitor) override;
    bool match0(const Type *otherType, Matcher *matcher) const override;
};

class CPLUSPLUS_EXPORT Enum: public Scope, public Type
{
public:
    Enum(TranslationUnit *translationUnit, int sourceLocation, const Name *name);
    Enum(Clone *clone, Subst *subst, Enum *original);
    virtual ~Enum();

    bool isScoped() const;
    void setScoped(bool scoped);

    // Symbol's interface
    FullySpecifiedType type() const override;

    const Enum *asEnum() const override
    { return this; }

    Enum *asEnum() override
    { return this; }

    // Type's interface
    const Enum *asEnumType() const override
    { return this; }

    Enum *asEnumType() override
    { return this; }

protected:
    void visitSymbol0(SymbolVisitor *visitor) override;
    void accept0(TypeVisitor *visitor) override;
    bool match0(const Type *otherType, Matcher *matcher) const override;

private:
    bool _isScoped;
};

class CPLUSPLUS_EXPORT Function: public Scope, public Type
{
public:
    enum MethodKey {
        NormalMethod,
        SlotMethod,
        SignalMethod,
        InvokableMethod
    };

    enum RefQualifier {
        NoRefQualifier, // a function declared w/o & and && => *this may be lvalue or rvalue
        LvalueRefQualifier, // a function declared with & => *this is lvalue
        RvalueRefQualifier // a function declared with && => *this is rvalue
    };

public:
    Function(TranslationUnit *translationUnit, int sourceLocation, const Name *name);
    Function(Clone *clone, Subst *subst, Function *original);
    virtual ~Function();

    bool isNormal() const;
    bool isSignal() const;
    bool isSlot() const;
    bool isInvokable() const;
    int methodKey() const;
    void setMethodKey(int key);

    FullySpecifiedType returnType() const;
    void setReturnType(const FullySpecifiedType &returnType);

    /** Convenience function that returns whether the function returns something (including void). */
    bool hasReturnType() const;

    int argumentCount() const;
    Symbol *argumentAt(int index) const;

    /** Convenience function that returns whether the function receives any arguments. */
    bool hasArguments() const;
    int minimumArgumentCount() const;

    bool isVirtual() const;
    void setVirtual(bool isVirtual);

    bool isOverride() const;
    void setOverride(bool isOverride);

    bool isFinal() const;
    void setFinal(bool isFinal);

    bool isVariadic() const;
    void setVariadic(bool isVariadic);

    bool isVariadicTemplate() const;
    void setVariadicTemplate(bool isVariadicTemplate);

    bool isConst() const;
    void setConst(bool isConst);

    bool isStatic() const { return f._isStatic; }
    void setStatic(bool isStatic) { f._isStatic = isStatic; }

    bool isVolatile() const;
    void setVolatile(bool isVolatile);

    bool isPureVirtual() const;
    void setPureVirtual(bool isPureVirtual);

    RefQualifier refQualifier() const;
    void setRefQualifier(RefQualifier refQualifier);

    bool isSignatureEqualTo(const Function *other, Matcher *matcher = nullptr) const;

    bool isAmbiguous() const; // internal
    void setAmbiguous(bool isAmbiguous); // internal

    bool maybeValidPrototype(int actualArgumentCount) const;

    const StringLiteral *exceptionSpecification();
    void setExceptionSpecification(const StringLiteral *spec);

    // Symbol's interface
    FullySpecifiedType type() const override;

    const Function *asFunction() const override
    { return this; }

    Function *asFunction() override
    { return this; }

    // Type's interface
    const Function *asFunctionType() const override
    { return this; }

    Function *asFunctionType() override
    { return this; }

protected:
    void visitSymbol0(SymbolVisitor *visitor) override;
    void accept0(TypeVisitor *visitor) override;
    bool match0(const Type *otherType, Matcher *matcher) const override;

private:
    FullySpecifiedType _returnType;
    const StringLiteral *_exceptionSpecification = nullptr;
    struct Flags {
        unsigned _isVirtual: 1;
        unsigned _isOverride: 1;
        unsigned _isFinal: 1;
        unsigned _isStatic: 1;
        unsigned _isVariadic: 1;
        unsigned _isVariadicTemplate: 1;
        unsigned _isPureVirtual: 1;
        unsigned _isConst: 1;
        unsigned _isVolatile: 1;
        unsigned _isAmbiguous: 1;
        unsigned _methodKey: 3;
        unsigned _refQualifier: 2;
    };
    union {
        unsigned _flags;
        Flags f;
    };
};

class CPLUSPLUS_EXPORT Template: public Scope, public Type
{
public:
    Template(TranslationUnit *translationUnit, int sourceLocation, const Name *name);
    Template(Clone *clone, Subst *subst, Template *original);
    virtual ~Template();

    int templateParameterCount() const;
    Symbol *templateParameterAt(int index) const;
    Symbol *declaration() const;

    // Symbol's interface
    FullySpecifiedType type() const override;

    const Template *asTemplate() const override
    { return this; }

    Template *asTemplate() override
    { return this; }

    // Type's interface
    const Template *asTemplateType() const override
    { return this; }

    Template *asTemplateType() override
    { return this; }

protected:
    void visitSymbol0(SymbolVisitor *visitor) override;
    void accept0(TypeVisitor *visitor) override;
    bool match0(const Type *otherType, Matcher *matcher) const override;
};


class CPLUSPLUS_EXPORT Namespace: public Scope, public Type
{
public:
    Namespace(TranslationUnit *translationUnit, int sourceLocation, const Name *name);
    Namespace(Clone *clone, Subst *subst, Namespace *original);
    virtual ~Namespace();

    // Symbol's interface
    FullySpecifiedType type() const override;

    const Namespace *asNamespace() const override
    { return this; }

    Namespace *asNamespace() override
    { return this; }

    // Type's interface
    const Namespace *asNamespaceType() const override
    { return this; }

    Namespace *asNamespaceType() override
    { return this; }

    bool isInline() const
    { return _isInline; }

    void setInline(bool onoff)
    { _isInline = onoff; }

protected:
    void visitSymbol0(SymbolVisitor *visitor) override;
    void accept0(TypeVisitor *visitor) override;
    bool match0(const Type *otherType, Matcher *matcher) const override;

private:
    bool _isInline;
};

class CPLUSPLUS_EXPORT BaseClass: public Symbol
{
public:
    BaseClass(TranslationUnit *translationUnit, int sourceLocation, const Name *name);
    BaseClass(Clone *clone, Subst *subst, BaseClass *original);
    virtual ~BaseClass();

    bool isVirtual() const;
    void setVirtual(bool isVirtual);

    bool isVariadic() const;
    void setVariadic(bool isVariadic);

    // Symbol's interface
    FullySpecifiedType type() const override;
    void setType(const FullySpecifiedType &type);

    const BaseClass *asBaseClass() const override
    { return this; }

    BaseClass *asBaseClass() override
    { return this; }

protected:
    void visitSymbol0(SymbolVisitor *visitor) override;

private:
    bool _isVariadic = false;
    bool _isVirtual;
    FullySpecifiedType _type;
};

class CPLUSPLUS_EXPORT Class: public Scope, public Type
{
public:
    Class(TranslationUnit *translationUnit, int sourceLocation, const Name *name);
    Class(Clone *clone, Subst *subst, Class *original);
    virtual ~Class();

    enum Key {
        ClassKey,
        StructKey,
        UnionKey
    };

    bool isClass() const;
    bool isStruct() const;
    bool isUnion() const;
    Key classKey() const;
    void setClassKey(Key key);

    int baseClassCount() const;
    BaseClass *baseClassAt(int index) const;
    void addBaseClass(BaseClass *baseClass);
    const std::vector<BaseClass *> &baseClasses() const { return _baseClasses; }

    // Symbol's interface
    FullySpecifiedType type() const override;

    const Class *asClass() const override
    { return this; }

    Class *asClass() override
    { return this; }

    // Type's interface
    const Class *asClassType() const override
    { return this; }

    Class *asClassType() override
    { return this; }

protected:
    void visitSymbol0(SymbolVisitor *visitor) override;
    void accept0(TypeVisitor *visitor) override;
    bool match0(const Type *otherType, Matcher *matcher) const override;

private:
    Key _key;
    std::vector<BaseClass *> _baseClasses;
};

class CPLUSPLUS_EXPORT QtPropertyDeclaration: public Symbol
{
public:
    enum Flag {
        NoFlags = 0,
        ReadFunction = 1 << 0,
        WriteFunction = 1 << 1,
        MemberVariable = 1 << 2,
        ResetFunction = 1 << 3,
        NotifyFunction = 1 << 4,
        DesignableFlag = 1 << 5,
        DesignableFunction = 1 << 6,
        ScriptableFlag = 1 << 7,
        ScriptableFunction = 1 << 8,
        StoredFlag = 1 << 9,
        StoredFunction = 1 << 10,
        UserFlag = 1 << 11,
        UserFunction = 1 << 12,
        ConstantFlag = 1 << 13,
        FinalFlag = 1 << 14
    };

public:
    QtPropertyDeclaration(TranslationUnit *translationUnit, int sourceLocation, const Name *name);
    QtPropertyDeclaration(Clone *clone, Subst *subst, QtPropertyDeclaration *original);
    virtual ~QtPropertyDeclaration();

    void setType(const FullySpecifiedType &type);

    void setFlags(int flags);
    int flags() const;

    // Symbol's interface
    FullySpecifiedType type() const override;

    const QtPropertyDeclaration *asQtPropertyDeclaration() const override
    { return this; }

    QtPropertyDeclaration *asQtPropertyDeclaration() override
    { return this; }

protected:
    void visitSymbol0(SymbolVisitor *visitor) override;

private:
    FullySpecifiedType _type;
    int _flags;
};

class CPLUSPLUS_EXPORT QtEnum: public Symbol
{
public:
    QtEnum(TranslationUnit *translationUnit, int sourceLocation, const Name *name);
    QtEnum(Clone *clone, Subst *subst, QtEnum *original);
    virtual ~QtEnum();

    // Symbol's interface
    FullySpecifiedType type() const override;

    const QtEnum *asQtEnum() const override
    { return this; }

    QtEnum *asQtEnum() override
    { return this; }

protected:
    void visitSymbol0(SymbolVisitor *visitor) override;
};

class CPLUSPLUS_EXPORT ObjCBaseClass: public Symbol
{
public:
    ObjCBaseClass(TranslationUnit *translationUnit, int sourceLocation, const Name *name);
    ObjCBaseClass(Clone *clone, Subst *subst, ObjCBaseClass *original);
    virtual ~ObjCBaseClass();

    // Symbol's interface
    FullySpecifiedType type() const override;

    const ObjCBaseClass *asObjCBaseClass() const override
    { return this; }

    ObjCBaseClass *asObjCBaseClass() override
    { return this; }

protected:
    void visitSymbol0(SymbolVisitor *visitor) override;
};

class CPLUSPLUS_EXPORT ObjCBaseProtocol: public Symbol
{
public:
    ObjCBaseProtocol(TranslationUnit *translationUnit, int sourceLocation, const Name *name);
    ObjCBaseProtocol(Clone *clone, Subst *subst, ObjCBaseProtocol *original);
    virtual ~ObjCBaseProtocol();

    // Symbol's interface
    FullySpecifiedType type() const override;

    const ObjCBaseProtocol *asObjCBaseProtocol() const override
    { return this; }

    ObjCBaseProtocol *asObjCBaseProtocol() override
    { return this; }

protected:
    void visitSymbol0(SymbolVisitor *visitor) override;
};

class CPLUSPLUS_EXPORT ObjCForwardProtocolDeclaration: public Symbol, public Type
{
public:
    ObjCForwardProtocolDeclaration(TranslationUnit *translationUnit, int sourceLocation, const Name *name);
    ObjCForwardProtocolDeclaration(Clone *clone, Subst *subst, ObjCForwardProtocolDeclaration *original);
    virtual ~ObjCForwardProtocolDeclaration();

    // Symbol's interface
    FullySpecifiedType type() const override;

    const ObjCForwardProtocolDeclaration *asObjCForwardProtocolDeclaration() const override
    { return this; }

    ObjCForwardProtocolDeclaration *asObjCForwardProtocolDeclaration() override
    { return this; }

    // Type's interface
    const ObjCForwardProtocolDeclaration *asObjCForwardProtocolDeclarationType() const override
    { return this; }

    ObjCForwardProtocolDeclaration *asObjCForwardProtocolDeclarationType() override
    { return this; }

protected:
    void visitSymbol0(SymbolVisitor *visitor) override;
    void accept0(TypeVisitor *visitor) override;
    bool match0(const Type *otherType, Matcher *matcher) const override;
};

class CPLUSPLUS_EXPORT ObjCProtocol: public Scope, public Type
{
public:
    ObjCProtocol(TranslationUnit *translationUnit, int sourceLocation, const Name *name);
    ObjCProtocol(Clone *clone, Subst *subst, ObjCProtocol *original);
    virtual ~ObjCProtocol();

    int protocolCount() const;
    ObjCBaseProtocol *protocolAt(int index) const;
    void addProtocol(ObjCBaseProtocol *protocol);

    // Symbol's interface
    FullySpecifiedType type() const override;

    const ObjCProtocol *asObjCProtocol() const override
    { return this; }

    ObjCProtocol *asObjCProtocol() override
    { return this; }

    // Type's interface
    const ObjCProtocol *asObjCProtocolType() const override
    { return this; }

    ObjCProtocol *asObjCProtocolType() override
    { return this; }

protected:
    void visitSymbol0(SymbolVisitor *visitor) override;
    void accept0(TypeVisitor *visitor) override;
    bool match0(const Type *otherType, Matcher *matcher) const override;

private:
    std::vector<ObjCBaseProtocol *> _protocols;
};

class CPLUSPLUS_EXPORT ObjCForwardClassDeclaration: public Symbol, public Type
{
public:
    ObjCForwardClassDeclaration(TranslationUnit *translationUnit, int sourceLocation, const Name *name);
    ObjCForwardClassDeclaration(Clone *clone, Subst *subst, ObjCForwardClassDeclaration *original);
    virtual ~ObjCForwardClassDeclaration();

    // Symbol's interface
    FullySpecifiedType type() const override;

    const ObjCForwardClassDeclaration *asObjCForwardClassDeclaration() const override
    { return this; }

    ObjCForwardClassDeclaration *asObjCForwardClassDeclaration() override
    { return this; }

    // Type's interface
    const ObjCForwardClassDeclaration *asObjCForwardClassDeclarationType() const override
    { return this; }

    ObjCForwardClassDeclaration *asObjCForwardClassDeclarationType() override
    { return this; }

protected:
    void visitSymbol0(SymbolVisitor *visitor) override;
    void accept0(TypeVisitor *visitor) override;
    bool match0(const Type *otherType, Matcher *matcher) const override;
};

class CPLUSPLUS_EXPORT ObjCClass: public Scope, public Type
{
public:
    ObjCClass(TranslationUnit *translationUnit, int sourceLocation, const Name *name);
    ObjCClass(Clone *clone, Subst *subst, ObjCClass *original);
    virtual ~ObjCClass();

    bool isInterface() const;
    void setInterface(bool isInterface);

    bool isCategory() const;
    const Name *categoryName() const;
    void setCategoryName(const Name *categoryName);

    ObjCBaseClass *baseClass() const;
    void setBaseClass(ObjCBaseClass *baseClass);

    int protocolCount() const;
    ObjCBaseProtocol *protocolAt(int index) const;
    void addProtocol(ObjCBaseProtocol *protocol);

    // Symbol's interface
    FullySpecifiedType type() const override;

    const ObjCClass *asObjCClass() const override
    { return this; }

    ObjCClass *asObjCClass() override
    { return this; }

    // Type's interface
    const ObjCClass *asObjCClassType() const override
    { return this; }

    ObjCClass *asObjCClassType() override
    { return this; }

protected:
    void visitSymbol0(SymbolVisitor *visitor) override;
    void accept0(TypeVisitor *visitor) override;
    bool match0(const Type *otherType, Matcher *matcher) const override;

private:
    const Name *_categoryName;
    ObjCBaseClass * _baseClass;
    std::vector<ObjCBaseProtocol *> _protocols;
    bool _isInterface;
};

class CPLUSPLUS_EXPORT ObjCMethod: public Scope, public Type
{
public:
    ObjCMethod(TranslationUnit *translationUnit, int sourceLocation, const Name *name);
    ObjCMethod(Clone *clone, Subst *subst, ObjCMethod *original);
    virtual ~ObjCMethod();

    FullySpecifiedType returnType() const;
    void setReturnType(const FullySpecifiedType &returnType);

    /** Convenience function that returns whether the function returns something (including void). */
    bool hasReturnType() const;

    int argumentCount() const;
    Symbol *argumentAt(int index) const;

    /** Convenience function that returns whether the function receives any arguments. */
    bool hasArguments() const;

    bool isVariadic() const;
    void setVariadic(bool isVariadic);

    // Symbol's interface
    FullySpecifiedType type() const override;

    const ObjCMethod *asObjCMethod() const override
    { return this; }

    ObjCMethod *asObjCMethod() override
    { return this; }

    // Type's interface
    const ObjCMethod *asObjCMethodType() const override
    { return this; }

    ObjCMethod *asObjCMethodType() override
    { return this; }

protected:
    void visitSymbol0(SymbolVisitor *visitor) override;
    void accept0(TypeVisitor *visitor) override;
    bool match0(const Type *otherType, Matcher *matcher) const override;

private:
    FullySpecifiedType _returnType;
    struct Flags {
        unsigned _isVariadic: 1;
    };
    union {
        unsigned _flags;
        Flags f;
    };
};

class CPLUSPLUS_EXPORT ObjCPropertyDeclaration: public Symbol
{
public:
    enum PropertyAttributes {
        None = 0,
        Assign = 1 << 0,
        Retain = 1 << 1,
        Copy = 1 << 2,
        ReadOnly = 1 << 3,
        ReadWrite = 1 << 4,
        Getter = 1 << 5,
        Setter = 1 << 6,
        NonAtomic = 1 << 7,

        WritabilityMask = ReadOnly | ReadWrite,
        SetterSemanticsMask = Assign | Retain | Copy
    };

public:
    ObjCPropertyDeclaration(TranslationUnit *translationUnit,
                            int sourceLocation,
                            const Name *name);
    ObjCPropertyDeclaration(Clone *clone, Subst *subst, ObjCPropertyDeclaration *original);
    virtual ~ObjCPropertyDeclaration();

    bool hasAttribute(int attribute) const;
    void setAttributes(int attributes);

    bool hasGetter() const;
    bool hasSetter() const;

    const Name *getterName() const;

    void setGetterName(const Name *getterName);

    const Name *setterName() const;
    void setSetterName(const Name *setterName);

    void setType(const FullySpecifiedType &type);

    // Symbol's interface
    FullySpecifiedType type() const override;

    const ObjCPropertyDeclaration *asObjCPropertyDeclaration() const override
    { return this; }

    ObjCPropertyDeclaration *asObjCPropertyDeclaration() override
    { return this; }

protected:
    void visitSymbol0(SymbolVisitor *visitor) override;

private:
    const Name *_getterName;
    const Name *_setterName;
    FullySpecifiedType _type;
    int _propertyAttributes;
};

} // namespace CPlusPlus
