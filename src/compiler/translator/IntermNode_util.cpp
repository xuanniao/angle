//
// Copyright (c) 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// IntermNode_util.cpp: High-level utilities for creating AST nodes and node hierarchies. Mostly
// meant to be used in AST transforms.

#include "compiler/translator/IntermNode_util.h"

namespace sh
{

namespace
{

TName GetInternalFunctionName(const char *name)
{
    TString nameStr(name);
    TName nameObj(nameStr);
    nameObj.setInternal(true);
    return nameObj;
}

}  // anonymous namespace

TIntermFunctionPrototype *CreateInternalFunctionPrototypeNode(const TType &returnType,
                                                              const char *name,
                                                              const TSymbolUniqueId &functionId)
{
    TIntermFunctionPrototype *functionNode = new TIntermFunctionPrototype(returnType, functionId);
    functionNode->getFunctionSymbolInfo()->setNameObj(GetInternalFunctionName(name));
    return functionNode;
}

TIntermFunctionDefinition *CreateInternalFunctionDefinitionNode(const TType &returnType,
                                                                const char *name,
                                                                TIntermBlock *functionBody,
                                                                const TSymbolUniqueId &functionId)
{
    TIntermFunctionPrototype *prototypeNode =
        CreateInternalFunctionPrototypeNode(returnType, name, functionId);
    return new TIntermFunctionDefinition(prototypeNode, functionBody);
}

TIntermAggregate *CreateInternalFunctionCallNode(const TType &returnType,
                                                 const char *name,
                                                 const TSymbolUniqueId &functionId,
                                                 TIntermSequence *arguments)
{
    TIntermAggregate *functionNode = TIntermAggregate::CreateFunctionCall(
        returnType, functionId, GetInternalFunctionName(name), arguments);
    return functionNode;
}

TIntermTyped *CreateZeroNode(const TType &type)
{
    TType constType(type);
    constType.setQualifier(EvqConst);

    if (!type.isArray() && type.getBasicType() != EbtStruct)
    {
        size_t size       = constType.getObjectSize();
        TConstantUnion *u = new TConstantUnion[size];
        for (size_t i = 0; i < size; ++i)
        {
            switch (type.getBasicType())
            {
                case EbtFloat:
                    u[i].setFConst(0.0f);
                    break;
                case EbtInt:
                    u[i].setIConst(0);
                    break;
                case EbtUInt:
                    u[i].setUConst(0u);
                    break;
                case EbtBool:
                    u[i].setBConst(false);
                    break;
                default:
                    // CreateZeroNode is called by ParseContext that keeps parsing even when an
                    // error occurs, so it is possible for CreateZeroNode to be called with
                    // non-basic types. This happens only on error condition but CreateZeroNode
                    // needs to return a value with the correct type to continue the typecheck.
                    // That's why we handle non-basic type by setting whatever value, we just need
                    // the type to be right.
                    u[i].setIConst(42);
                    break;
            }
        }

        TIntermConstantUnion *node = new TIntermConstantUnion(u, constType);
        return node;
    }

    if (type.getBasicType() == EbtVoid)
    {
        // Void array. This happens only on error condition, similarly to the case above. We don't
        // have a constructor operator for void, so this needs special handling. We'll end up with a
        // value without the array type, but that should not be a problem.
        constType.clearArrayness();
        return CreateZeroNode(constType);
    }

    TIntermSequence *arguments = new TIntermSequence();

    if (type.isArray())
    {
        TType elementType(type);
        elementType.clearArrayness();

        size_t arraySize = type.getArraySize();
        for (size_t i = 0; i < arraySize; ++i)
        {
            arguments->push_back(CreateZeroNode(elementType));
        }
    }
    else
    {
        ASSERT(type.getBasicType() == EbtStruct);

        TStructure *structure = type.getStruct();
        for (const auto &field : structure->fields())
        {
            arguments->push_back(CreateZeroNode(*field->type()));
        }
    }

    return TIntermAggregate::CreateConstructor(constType, arguments);
}

TIntermConstantUnion *CreateIndexNode(int index)
{
    TConstantUnion *u = new TConstantUnion[1];
    u[0].setIConst(index);

    TType type(EbtInt, EbpUndefined, EvqConst, 1);
    TIntermConstantUnion *node = new TIntermConstantUnion(u, type);
    return node;
}

TIntermConstantUnion *CreateBoolNode(bool value)
{
    TConstantUnion *u = new TConstantUnion[1];
    u[0].setBConst(value);

    TType type(EbtBool, EbpUndefined, EvqConst, 1);
    TIntermConstantUnion *node = new TIntermConstantUnion(u, type);
    return node;
}

TIntermBlock *EnsureBlock(TIntermNode *node)
{
    if (node == nullptr)
        return nullptr;
    TIntermBlock *blockNode = node->getAsBlock();
    if (blockNode != nullptr)
        return blockNode;

    blockNode = new TIntermBlock();
    blockNode->setLine(node->getLine());
    blockNode->appendStatement(node);
    return blockNode;
}

}  // namespace sh
