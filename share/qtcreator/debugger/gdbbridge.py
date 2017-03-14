############################################################################
#
# Copyright (C) 2016 The Qt Company Ltd.
# Contact: https://www.qt.io/licensing/
#
# This file is part of Qt Creator.
#
# Commercial License Usage
# Licensees holding valid commercial Qt licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and The Qt Company. For licensing terms
# and conditions see https://www.qt.io/terms-conditions. For further
# information use the contact form at https://www.qt.io/contact-us.
#
# GNU General Public License Usage
# Alternatively, this file may be used under the terms of the GNU
# General Public License version 3 as published by the Free Software
# Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
# included in the packaging of this file. Please review the following
# information to ensure the GNU General Public License requirements will
# be met: https://www.gnu.org/licenses/gpl-3.0.html.
#
############################################################################

try:
    import __builtin__
except:
    import builtins

import gdb
import os
import os.path
import sys
import struct
import types

from dumper import *


#######################################################################
#
# Infrastructure
#
#######################################################################

def safePrint(output):
    try:
        print(output)
    except:
        out = ''
        for c in output:
            cc = ord(c)
            if cc > 127:
                out += '\\\\%d' % cc
            elif cc < 0:
                out += '\\\\%d' % (cc + 256)
            else:
                out += c
        print(out)

def registerCommand(name, func):

    class Command(gdb.Command):
        def __init__(self):
            super(Command, self).__init__(name, gdb.COMMAND_OBSCURE)
        def invoke(self, args, from_tty):
            safePrint(func(args))

    Command()



#######################################################################
#
# Convenience
#
#######################################################################

# Just convienience for 'python print ...'
class PPCommand(gdb.Command):
    def __init__(self):
        super(PPCommand, self).__init__('pp', gdb.COMMAND_OBSCURE)
    def invoke(self, args, from_tty):
        print(eval(args))

PPCommand()

# Just convienience for 'python print gdb.parse_and_eval(...)'
class PPPCommand(gdb.Command):
    def __init__(self):
        super(PPPCommand, self).__init__('ppp', gdb.COMMAND_OBSCURE)
    def invoke(self, args, from_tty):
        print(gdb.parse_and_eval(args))

PPPCommand()


def scanStack(p, n):
    p = int(p)
    r = []
    for i in xrange(n):
        f = gdb.parse_and_eval('{void*}%s' % p)
        m = gdb.execute('info symbol %s' % f, to_string=True)
        if not m.startswith('No symbol matches'):
            r.append(m)
        p += f.type.sizeof
    return r

class ScanStackCommand(gdb.Command):
    def __init__(self):
        super(ScanStackCommand, self).__init__('scanStack', gdb.COMMAND_OBSCURE)
    def invoke(self, args, from_tty):
        if len(args) == 0:
            args = 20
        safePrint(scanStack(gdb.parse_and_eval('$sp'), int(args)))

ScanStackCommand()


#######################################################################
#
# Import plain gdb pretty printers
#
#######################################################################

class PlainDumper:
    def __init__(self, printer):
        self.printer = printer
        self.typeCache = {}

    def __call__(self, d, value):
        try:
            printer = self.printer.gen_printer(value)
        except:
            printer = self.printer.invoke(value)
        lister = getattr(printer, 'children', None)
        children = [] if lister is None else list(lister())
        d.putType(self.printer.name)
        val = printer.to_string()
        if isinstance(val, str):
            d.putValue(val)
        elif sys.version_info[0] <= 2 and isinstance(val, unicode):
            d.putValue(val)
        else: # Assuming LazyString
            d.putCharArrayHelper(val.address, val.length, val.type)

        d.putNumChild(len(children))
        if d.isExpanded():
            with Children(d):
                for child in children:
                    d.putSubItem(child[0], child[1])

def importPlainDumpers(args):
    if args == 'off':
        try:
            gdb.execute('disable pretty-printer .* .*')
        except:
            # Might occur in non-ASCII directories
            warn('COULD NOT DISABLE PRETTY PRINTERS')
    else:
        theDumper.importPlainDumpers()

registerCommand('importPlainDumpers', importPlainDumpers)



class OutputSafer:
    def __init__(self, d):
        self.d = d

    def __enter__(self):
        self.savedOutput = self.d.output
        self.d.output = []

    def __exit__(self, exType, exValue, exTraceBack):
        if self.d.passExceptions and not exType is None:
            showException('OUTPUTSAFER', exType, exValue, exTraceBack)
            self.d.output = self.savedOutput
        else:
            self.savedOutput.extend(self.d.output)
            self.d.output = self.savedOutput
        return False



#######################################################################
#
# The Dumper Class
#
#######################################################################


class Dumper(DumperBase):

    def __init__(self):
        DumperBase.__init__(self)

        # These values will be kept between calls to 'fetchVariables'.
        self.isGdb = True
        self.typeCache = {}
        self.interpreterBreakpointResolvers = []

    def prepare(self, args):
        self.output = []
        self.setVariableFetchingOptions(args)

    def fromFrameValue(self, nativeValue):
        #warn('FROM FRAME VALUE: %s' % nativeValue.address)
        val = nativeValue
        try:
           val = nativeValue.cast(nativeValue.dynamic_type)
        except:
           pass
        return self.fromNativeValue(val)

    def fromNativeValue(self, nativeValue):
        #warn('FROM NATIVE VALUE: %s' % nativeValue)
        self.check(isinstance(nativeValue, gdb.Value))
        nativeType = nativeValue.type
        code = nativeType.code
        if code == gdb.TYPE_CODE_REF:
            targetType = self.fromNativeType(nativeType.target().unqualified(), nativeValue)
            val = self.createReferenceValue(toInteger(nativeValue.address), targetType)
            #warn('CREATED REF: %s' % val)
            return val
        if code == gdb.TYPE_CODE_PTR:
            try:
                nativeTargetValue = nativeValue.dereference()
            except:
                nativeTargetValue = None
            targetType = self.fromNativeType(nativeType.target().unqualified(), nativeTargetValue)
            val = self.createPointerValue(toInteger(nativeValue), targetType)
            #warn('CREATED PTR 1: %s' % val)
            if not nativeValue.address is None:
                val.laddress = toInteger(nativeValue.address)
            #warn('CREATED PTR 2: %s' % val)
            return val
        if code == gdb.TYPE_CODE_TYPEDEF:
            targetType = nativeType.strip_typedefs().unqualified()
            #warn('TARGET TYPE: %s' % targetType)
            if targetType.code == gdb.TYPE_CODE_ARRAY:
                val = self.Value(self)
                val.laddress = toInteger(nativeValue.address)
            else:
                # Cast may fail (e.g for arrays, see test for Bug5799)
                val = self.fromNativeValue(nativeValue.cast(targetType))
            val.type = self.fromNativeType(nativeType, nativeValue)
            #warn('CREATED TYPEDEF: %s' % val)
            return val

        val = self.Value(self)
        if not nativeValue.address is None:
            val.laddress = toInteger(nativeValue.address)
        else:
            size = nativeType.sizeof
            chars = self.lookupNativeType('unsigned char')
            y = nativeValue.cast(chars.array(0, int(nativeType.sizeof - 1)))
            buf = bytearray(struct.pack('x' * size))
            for i in range(size):
                buf[i] = int(y[i])
            val.ldata = bytes(buf)

        val.type = self.fromNativeType(nativeType, nativeValue)
        val.lIsInScope = not nativeValue.is_optimized_out
        code = nativeType.code
        if code == gdb.TYPE_CODE_ENUM:
            val.ldisplay = str(nativeValue)
            intval = int(nativeValue)
            if val.ldisplay != intval:
                val.ldisplay += ' (%s)' % intval
        elif code == gdb.TYPE_CODE_COMPLEX:
            val.ldisplay = str(nativeValue)
        #elif code == gdb.TYPE_CODE_ARRAY:
        #    val.type.ltarget = nativeValue[0].type.unqualified()
        return val

    def ptrSize(self):
        result = gdb.lookup_type('void').pointer().sizeof
        self.ptrSize = lambda: result
        return result

    def fromNativeType(self, nativeType, nativeValue = None):
        self.check(isinstance(nativeType, gdb.Type))
        code = nativeType.code
        #warn('FROM NATIVE TYPE: %s' % nativeType)
        nativeType = nativeType.unqualified()

        if code == gdb.TYPE_CODE_PTR:
            #warn('PTR')
            if nativeValue is not None:
                nativeValue = nativeValue.dereference()
            targetType = self.fromNativeType(nativeType.target().unqualified(), nativeValue)
            return self.createPointerType(targetType)

        if code == gdb.TYPE_CODE_REF:
            #warn('REF')
            targetType = self.fromNativeType(nativeType.target().unqualified())
            return self.createReferenceType(targetType)

        if code == gdb.TYPE_CODE_ARRAY:
            #warn('ARRAY')
            nativeTargetType = nativeType.target().unqualified()
            targetType = self.fromNativeType(nativeTargetType)
            count = nativeType.sizeof // nativeTargetType.sizeof
            return self.createArrayType(targetType, count)

        if code == gdb.TYPE_CODE_TYPEDEF:
            #warn('TYPEDEF')
            nativeTargetType = nativeType.unqualified()
            while nativeTargetType.code == gdb.TYPE_CODE_TYPEDEF:
                nativeTargetType = nativeTargetType.strip_typedefs().unqualified()
            targetType = self.fromNativeType(nativeTargetType)
            return self.createTypedefedType(targetType, str(nativeType))

        if code == gdb.TYPE_CODE_ERROR:
            warn('Type error: %s' % nativeType)
            return self.Type(self, '')

        typeId = self.nativeTypeId(nativeType)
        res = self.typeData.get(typeId, None)
        if res is None:
            tdata = self.TypeData(self)
            tdata.name = str(nativeType)
            tdata.typeId = typeId
            tdata.lbitsize = nativeType.sizeof * 8
            tdata.code = {
                #gdb.TYPE_CODE_TYPEDEF : TypeCodeTypedef, # Handled above.
                gdb.TYPE_CODE_METHOD : TypeCodeFunction,
                gdb.TYPE_CODE_VOID : TypeCodeVoid,
                gdb.TYPE_CODE_FUNC : TypeCodeFunction,
                gdb.TYPE_CODE_METHODPTR : TypeCodeFunction,
                gdb.TYPE_CODE_MEMBERPTR : TypeCodeFunction,
                #gdb.TYPE_CODE_PTR : TypeCodePointer,  # Handled above.
                #gdb.TYPE_CODE_REF : TypeCodeReference,  # Handled above.
                gdb.TYPE_CODE_BOOL : TypeCodeIntegral,
                gdb.TYPE_CODE_CHAR : TypeCodeIntegral,
                gdb.TYPE_CODE_INT : TypeCodeIntegral,
                gdb.TYPE_CODE_FLT : TypeCodeFloat,
                gdb.TYPE_CODE_ENUM : TypeCodeEnum,
                #gdb.TYPE_CODE_ARRAY : TypeCodeArray,
                gdb.TYPE_CODE_STRUCT : TypeCodeStruct,
                gdb.TYPE_CODE_UNION : TypeCodeStruct,
                gdb.TYPE_CODE_COMPLEX : TypeCodeComplex,
                gdb.TYPE_CODE_STRING : TypeCodeFortranString,
            }[code]
            if tdata.code == TypeCodeEnum:
                tdata.enumDisplay = lambda intval : \
                    self.nativeTypeEnumDisplay(nativeType, intval)
            if tdata.code == TypeCodeStruct:
                tdata.lalignment = lambda : \
                    self.nativeStructAlignment(nativeType)
                tdata.lfields = self.listMembers(nativeType, nativeValue)
                #tdata.lfieldByName = lambda name : \
                #    self.nativeTypeFieldTypeByName(nativeType, name)
            tdata.templateArguments = self.listTemplateParameters(nativeType)
            self.registerType(typeId, tdata) # Fix up fields and template args
        #    warn('CREATE TYPE: %s' % typeId)
        #else:
        #    warn('REUSE TYPE: %s' % typeId)
        return self.Type(self, typeId)

    def listTemplateParameters(self, nativeType):
        targs = []
        pos = 0
        while True:
            try:
                targ = nativeType.template_argument(pos)
            except:
                break
            if isinstance(targ, gdb.Type):
                targs.append(self.fromNativeType(targ.unqualified()))
            elif isinstance(targ, gdb.Value):
                targs.append(self.fromNativeValue(targ).value())
            else:
                error('UNKNOWN TEMPLATE PARAMETER')
            pos += 1
        return targs

    def nativeTypeEnumDisplay(self, nativeType, intval):
        try:
            val = gdb.parse_and_eval('(%s)%d' % (nativeType, intval))
            return  '%s (%d)' % (val, intval)
        except:
            return '%d' % intval

    def nativeTypeId(self, nativeType):
        name = str(nativeType)
        if len(name) == 0:
            c = '0'
        elif name == 'union {...}':
            c = 'u'
        elif name.endswith('{...}'):
            c = 's'
        else:
            return name
        typeId = c + ''.join(['{%s:%s}' % (f.name, self.nativeTypeId(f.type))
                              for f in nativeType.fields()])
        return typeId

    def nativeStructAlignment(self, nativeType):
        self.preping('align ' + str(nativeType))
        #warn('NATIVE ALIGN FOR %s' % nativeType.name)
        def handleItem(nativeFieldType, align):
            a = self.fromNativeType(nativeFieldType).alignment()
            return a if a > align else align
        align = 1
        for f in nativeType.fields():
            align = handleItem(f.type, align)
        self.ping('align ' + str(nativeType))
        return align


        #except:
        #    # Happens in the BoostList dumper for a 'const bool'
        #    # item named 'constant_time_size'. There isn't anything we can do
        #    # in this case.
         #   pass

        #yield value.extractField(field)

    def fromNativeField(self, nativeField):
        field = self.Field(self)
        field.name = nativeField.name
        field.isBaseClass = nativeField.is_base_class
        # Something without a name.
        # Anonymous union? We need a dummy name to distinguish
        # multiple anonymous unions in the struct.
        # Since GDB commit b5b08fb4 anonymous structs get also reported
        # with a 'None' name.
        if field.name is not None and len(field.name) == 0:
            field.name = None
        nativeFieldType = nativeField.type.unqualified()

        if hasattr(nativeField, 'bitpos'):
            field.lbitpos = nativeField.bitpos

        if hasattr(nativeField, 'bitsize') and nativeField.bitsize != 0:
            field.lbitsize = nativeField.bitsize
        else:
            field.lbitsize = 8 * nativeFieldType.sizeof

        if field.lbitsize != nativeFieldType.sizeof * 8:
            field.ltype = self.createBitfieldType(str(nativeFieldType), field.lbitsize)
        else:
            field.ltype = self.fromNativeType(nativeFieldType)
        return field

    def memberFromField(self, nativeValue, nativeField):
        if nativeField.is_base_class:
            return nativeValue.cast(nativeField.type)
        try:
            return nativeValue[nativeField]
        except:
            pass
        try:
            return nativeValue[nativeField.name]
        except:
            pass
        return None

    def listMembers(self, nativeType, nativeValue):
        #warn('LISTING MEMBERS OF %s' % nativeType)
        try:
            nativeValueAddress = toInteger(nativeValue.address)
        except:
            nativeValueAddress = None

        if nativeValue is None or nativeValueAddress is None:
            return lambda value : self.listDelayedMembers(nativeType, value)

        def needsDeep(someNativeType):
            if someNativeType.sizeof >= ptrSize:
                for nativeField in someNativeType.fields():
                    if nativeField.is_base_class:
                        if nativeField.bitpos < 0 or needsDeep(nativeField.type):
                            return True
            return False

        ptrSize = self.ptrSize()
        if needsDeep(nativeType):
            return self.listDeepMembers(nativeType, nativeValue, nativeValueAddress)

        return self.listSimpleMembers(nativeType, nativeValue, nativeValueAddress)

    def listSimpleMembers(self, nativeType, nativeValue, nativeValueAddress):
        #warn('SIMPLE FIELDS FOR %s' % nativeType)
        fields = []
        for nativeField in nativeType.fields():
            nativeFieldType = nativeField.type.unqualified()
            #warn('SIMPLE FIELD: %s' % nativeField.name)
            #warn('  BITSIZE: %s' % nativeField.bitsize)
            #warn('  NAME: %s' % nativeField.name)
            #warn('  TYPE: %s' % nativeFieldType)
            #warn('  TYPEID: %s' % self.nativeTypeId(nativeFieldType))
            field = self.fromNativeField(nativeField)
            if field.ltype.code != TypeCodeBitfield:
                nativeMember = self.memberFromField(nativeValue, nativeField)
                if nativeMember is None:
                    warn('CANNOT EXTRACT FIELD %s FROM %s' % (nativeField.name, nativeValue))
                    #continue
                if nativeMember.address is None: # Could be a static member.
                    warn('NO ADDRESS OF %s' % nativeMember)
                    continue
                nativeMemberAddress = toInteger(nativeMember.address)
                offset = nativeMemberAddress - nativeValueAddress
                field.lbitpos = 8 * offset
            fields.append(field)
        return fields

    def listDelayedMembers(self, nativeType, value):
        #warn('DELAYED FIELDS FOR %s' % nativeType)
        if value.laddress is None:
            error('No address for object of type %s' % nativeType)
        nativeTypePointer = nativeType.unqualified().pointer()
        nativeValue = gdb.Value(value.laddress).cast(nativeTypePointer).dereference()
        return self.listSimpleMembers(nativeType, nativeValue, value.laddress)

    # The is expensive as it recurses through all base classes. Ideally this
    # should only be used if virtual inheritance is known to be involved.
    def listDeepMembers(self, nativeType, nativeValue, nativeBaseAddress, bpbase = 0):
        #warn('DEEP FIELDS FOR %s AT 0x%x' % (nativeType, nativeBaseAddress))
        fields = []
        for nativeField in nativeType.fields():
            nativeFieldType = nativeField.type.unqualified()
            #warn('DEEP FIELD: %s' % nativeField.name)
            #warn('  SIZE: %s' % nativeField.bitsize)
            #warn('  POS: %s' % nativeField.bitpos)
            #warn('  BPBASE: %s' % bpbase)
            #warn('  ARTIFICIAL: %s' % nativeField.artificial)
            #warn('  NAME: %s' % nativeField.name)
            #warn('  TYPE: %s' % nativeFieldType)
            #warn('  TYPEID: %s' % self.nativeTypeId(nativeFieldType))
            nativeMember = self.memberFromField(nativeValue, nativeField)
            if nativeField.is_base_class:
                #warn('  THIS IS VIRTUAL BASE OF TYPE %s' % nativeField.type)
                # This is a virtual base, we need to recurse *now*, as later
                # there is no way to generate a nativeValue with this type.
                #warn('   NATIVE MEMBER: %s' % nativeMember)
                pureFieldType = self.fromNativeType(nativeFieldType)
                fieldTypeId = pureFieldType.typeId + ' in ' + str(nativeType)
                fieldTypeData = pureFieldType.typeData().copy()
                bp = - nativeField.bitpos
                fieldTypeData.lfields = self.listDeepMembers(
                    nativeField.type, nativeMember, nativeBaseAddress, bpbase + bp - 64)
                self.registerType(fieldTypeId, fieldTypeData)

                field = self.Field(self)
                field.name = nativeField.name
                #field.name = fieldTypeId
                field.ltype = self.Type(self, fieldTypeId)
                field.isBaseClass = True
                field.isArtificial = True
                field.lbitsize = None
                field.lbitpos = None

                fields.append(field)
            else:
                # This is a normal field or non-virtual base.
                field = self.fromNativeField(nativeField)
                if field.ltype.code != TypeCodeBitfield:
                    #if field.lbitpos is None:
                    try:
                        nativeMemberAddress = toInteger(nativeMember.address)
                        #warn('  NATIVE MEMBER ADDRESS: 0x%x' % nativeMemberAddress)
                        offset = nativeMemberAddress - nativeBaseAddress
                        #offset = bpbase + nativeField.bitpos
                        #warn('DEEP OFFSET: %s  POS: %s' % (offset // 8, field.lbitpos // 8))
                        field.lbitpos = offset * 8
                    except:
                        field.lbitpos = None
                        field.lvalue = 'XXX'
                    #else:
                    #    warn('REUSING BITPOS %s FOR %s' % (field.lbitpos, field.name))
                fields.append(field)
        return fields


    def listLocals(self, partialVar):
        frame = gdb.selected_frame()

        try:
            block = frame.block()
            #warn('BLOCK: %s ' % block)
        except RuntimeError as error:
            #warn('BLOCK IN FRAME NOT ACCESSIBLE: %s' % error)
            return []
        except:
            warn('BLOCK NOT ACCESSIBLE FOR UNKNOWN REASONS')
            return []

        items = []
        shadowed = {}
        while True:
            if block is None:
                warn("UNEXPECTED 'None' BLOCK")
                break
            for symbol in block:

              # Filter out labels etc.
              if symbol.is_variable or symbol.is_argument:
                name = symbol.print_name

                if name in ('__in_chrg', '__PRETTY_FUNCTION__'):
                    continue

                if not partialVar is None and partialVar != name:
                    continue

                # 'NotImplementedError: Symbol type not yet supported in
                # Python scripts.'
                #warn('SYMBOL %s  (%s, %s)): ' % (symbol, name, symbol.name))
                if self.passExceptions and not self.isTesting:
                    nativeValue = frame.read_var(name, block)
                    value = self.fromFrameValue(nativeValue)
                    value.name = name
                    #warn('READ 0: %s' % value.stringify())
                    items.append(value)
                    continue

                try:
                    # Same as above, but for production.
                    nativeValue = frame.read_var(name, block)
                    value = self.fromFrameValue(nativeValue)
                    value.name = name
                    #warn('READ 1: %s' % value.stringify())
                    items.append(value)
                    continue
                except:
                    pass

                try:
                    #warn('READ 2: %s' % item.value)
                    value = self.fromFrameValue(frame.read_var(name))
                    value.name = name
                    items.append(value)
                    continue
                except:
                    # RuntimeError: happens for
                    #     void foo() { std::string s; std::wstring w; }
                    # ValueError: happens for (as of 2010/11/4)
                    #     a local struct as found e.g. in
                    #     gcc sources in gcc.c, int execute()
                    pass

                try:
                    #warn('READ 3: %s %s' % (name, item.value))
                    #warn('ITEM 3: %s' % item.value)
                    value = self.fromFrameValue(gdb.parse_and_eval(name))
                    value.name = name
                    items.append(value)
                except:
                    # Can happen in inlined code (see last line of
                    # RowPainter::paintChars(): 'RuntimeError:
                    # No symbol '__val' in current context.\n'
                    pass

            # The outermost block in a function has the function member
            # FIXME: check whether this is guaranteed.
            if not block.function is None:
                break

            block = block.superblock

        return items

    # Hack to avoid QDate* dumper timeouts with GDB 7.4 on 32 bit
    # due to misaligned %ebx in SSE calls (qstring.cpp:findChar)
    # This seems to be fixed in 7.9 (or earlier)
    def canCallLocale(self):
        return self.ptrSize() == 8

    def fetchVariables(self, args):
        self.resetStats()
        self.prepare(args)

        self.preping('endian')
        self.isBigEndian = gdb.execute('show endian', to_string = True).find('big endian') > 0
        self.ping('endian')
        self.packCode = '>' if self.isBigEndian else '<'

        (ok, res) = self.tryFetchInterpreterVariables(args)
        if ok:
            safePrint(res)
            return

        self.output.append('data=[')

        partialVar = args.get('partialvar', '')
        isPartial = len(partialVar) > 0
        partialName = partialVar.split('.')[1].split('@')[0] if isPartial else None

        variables = self.listLocals(partialName)
        #warn('VARIABLES: %s' % variables)

        # Take care of the return value of the last function call.
        if len(self.resultVarName) > 0:
            try:
                value = self.parseAndEvaluate(self.resultVarName)
                value.name = self.resultVarName
                value.iname = 'return.' + self.resultVarName
                variables.append(value)
            except:
                # Don't bother. It's only supplementary information anyway.
                pass

        self.handleLocals(variables)
        self.handleWatches(args)

        self.output.append('],typeinfo=[')
        for name in self.typesToReport.keys():
            typeobj = self.typesToReport[name]
            # Happens e.g. for '(anonymous namespace)::InsertDefOperation'
            #if not typeobj is None:
            #    self.output.append('{name="%s",size="%s"}'
            #        % (self.hexencode(name), typeobj.sizeof))
        self.output.append(']')
        self.typesToReport = {}

        if self.forceQtNamespace:
            self.qtNamepaceToReport = self.qtNamespace()

        if self.qtNamespaceToReport:
            self.output.append(',qtnamespace="%s"' % self.qtNamespaceToReport)
            self.qtNamespaceToReport = None

        self.output.append(',partial="%d"' % isPartial)
        self.output.append(',counts=%s' % self.counts)
        self.output.append(',timimgs=%s' % self.timings)

        tt = time.time()
        safePrint(''.join(self.output))
        print(',time="%d"' % int(1000 * (tt - time.time())))

    def parseAndEvaluate(self, exp):
        #warn('EVALUATE "%s"' % exp)
        try:
            val = gdb.parse_and_eval(exp)
        except RuntimeError as error:
            if self.passExceptions:
                warn("Cannot evaluate '%s': %s" % (exp, error))
            return None
        return self.fromNativeValue(val)

    def callHelper(self, rettype, value, function, args):
        # args is a tuple.
        arg = ''
        for i in range(len(args)):
            if i:
                arg += ','
            a = args[i]
            if (':' in a) and not ("'" in a):
                arg = "'%s'" % a
            else:
                arg += a

        #warn('CALL: %s -> %s(%s)' % (value, function, arg))
        typeName = value.type.name
        if typeName.find(':') >= 0:
            typeName = "'" + typeName + "'"
        # 'class' is needed, see http://sourceware.org/bugzilla/show_bug.cgi?id=11912
        #exp = '((class %s*)%s)->%s(%s)' % (typeName, value.laddress, function, arg)
        addr = value.laddress
        if addr is None:
           addr = self.pokeValue(value)
        #warn('PTR: %s -> %s(%s)' % (value, function, addr))
        exp = '((%s*)0x%x)->%s(%s)' % (typeName, addr, function, arg)
        #warn('CALL: %s' % exp)
        result = gdb.parse_and_eval(exp)
        #warn('  -> %s' % result)
        res = self.fromNativeValue(result)
        if value.laddress is None:
            self.releaseValue(addr)
        return res

    def makeExpression(self, value):
        typename = '::' + value.type.name
        #warn('  TYPE: %s' % typename)
        exp = '(*(%s*)(0x%x))' % (typename, value.address())
        #warn('  EXP: %s' % exp)
        return exp

    def makeStdString(init):
        # Works only for small allocators, but they are usually empty.
        gdb.execute('set $d=(std::string*)calloc(sizeof(std::string), 2)');
        gdb.execute('call($d->basic_string("' + init +
            '",*(std::allocator<char>*)(1+$d)))')
        value = gdb.parse_and_eval('$d').dereference()
        return value

    def pokeValue(self, value):
        # Allocates inferior memory and copies the contents of value.
        # Returns a pointer to the copy.
        # Avoid malloc symbol clash with QVector
        size = value.type.size()
        data = value.data()
        h = self.hexencode(data)
        #warn('DATA: %s' % h)
        string = ''.join('\\x' + h[2*i:2*i+2] for i in range(size))
        exp = '(%s*)memcpy(calloc(%d, 1), "%s", %d)' \
            % (value.type.name, size, string, size)
        #warn('EXP: %s' % exp)
        res = gdb.parse_and_eval(exp)
        #warn('RES: %s' % res)
        return toInteger(res)

    def releaseValue(self, address):
        gdb.parse_and_eval('free(0x%x)' % address)

    def setValue(self, address, typename, value):
        cmd = 'set {%s}%s=%s' % (typename, address, value)
        gdb.execute(cmd)

    def setValues(self, address, typename, values):
        cmd = 'set {%s[%s]}%s={%s}' \
            % (typename, len(values), address, ','.join(map(str, values)))
        gdb.execute(cmd)

    def selectedInferior(self):
        try:
            # gdb.Inferior is new in gdb 7.2
            self.cachedInferior = gdb.selected_inferior()
        except:
            # Pre gdb 7.4. Right now we don't have more than one inferior anyway.
            self.cachedInferior = gdb.inferiors()[0]

        # Memoize result.
        self.selectedInferior = lambda: self.cachedInferior
        return self.cachedInferior

    def readRawMemory(self, address, size):
        #warn('READ: %s FROM 0x%x' % (size, address))
        if address == 0 or size == 0:
            return bytes()
        self.preping('readMem')
        res = self.selectedInferior().read_memory(address, size)
        self.ping('readMem')
        return res

    def findStaticMetaObject(self, typename):
        symbolName = typename + '::staticMetaObject'
        symbol = gdb.lookup_global_symbol(symbolName, gdb.SYMBOL_VAR_DOMAIN)
        if not symbol:
            return 0
        try:
            # Older GDB ~7.4 don't have gdb.Symbol.value()
            return toInteger(symbol.value().address)
        except:
            pass

        address = gdb.parse_and_eval("&'%s'" % symbolName)
        return toInteger(address)

    def put(self, value):
        self.output.append(value)

    def isArmArchitecture(self):
        return 'arm' in gdb.TARGET_CONFIG.lower()

    def isQnxTarget(self):
        return 'qnx' in gdb.TARGET_CONFIG.lower()

    def isWindowsTarget(self):
        # We get i686-w64-mingw32
        return 'mingw' in gdb.TARGET_CONFIG.lower()

    def isMsvcTarget(self):
        return False

    def prettySymbolByAddress(self, address):
        try:
            return str(gdb.parse_and_eval('(void(*))0x%x' % address))
        except:
            return '0x%x' % address

    def qtVersionString(self):
        try:
            return str(gdb.lookup_symbol('qVersion')[0].value()())
        except:
            pass
        try:
            ns = self.qtNamespace()
            return str(gdb.parse_and_eval("((const char*(*)())'%sqVersion')()" % ns))
        except:
            pass
        return None

    def qtVersion(self):
        try:
            # Only available with Qt 5.3+
            qtversion = int(str(gdb.parse_and_eval('((void**)&qtHookData)[2]')), 16)
            self.qtVersion = lambda: qtversion
            return qtversion
        except:
            pass

        try:
            version = self.qtVersionString()
            (major, minor, patch) = version[version.find('"')+1:version.rfind('"')].split('.')
            qtversion = 0x10000 * int(major) + 0x100 * int(minor) + int(patch)
            self.qtVersion = lambda: qtversion
            return qtversion
        except:
            # Use fallback until we have a better answer.
            return self.fallbackQtVersion

    def createSpecialBreakpoints(self, args):
        self.specialBreakpoints = []
        def newSpecial(spec):
            class SpecialBreakpoint(gdb.Breakpoint):
                def __init__(self, spec):
                    super(SpecialBreakpoint, self).\
                        __init__(spec, gdb.BP_BREAKPOINT, internal=True)
                    self.spec = spec

                def stop(self):
                    print("Breakpoint on '%s' hit." % self.spec)
                    return True
            return SpecialBreakpoint(spec)

        # FIXME: ns is accessed too early. gdb.Breakpoint() has no
        # 'rbreak' replacement, and breakpoints created with
        # 'gdb.execute('rbreak...') cannot be made invisible.
        # So let's ignore the existing of namespaced builds for this
        # fringe feature here for now.
        ns = self.qtNamespace()
        if args.get('breakonabort', 0):
            self.specialBreakpoints.append(newSpecial('abort'))

        if args.get('breakonwarning', 0):
            self.specialBreakpoints.append(newSpecial(ns + 'qWarning'))
            self.specialBreakpoints.append(newSpecial(ns + 'QMessageLogger::warning'))

        if args.get('breakonfatal', 0):
            self.specialBreakpoints.append(newSpecial(ns + 'qFatal'))
            self.specialBreakpoints.append(newSpecial(ns + 'QMessageLogger::fatal'))

    #def threadname(self, maximalStackDepth, objectPrivateType):
    #    e = gdb.selected_frame()
    #    out = ''
    #    ns = self.qtNamespace()
    #    while True:
    #        maximalStackDepth -= 1
    #        if maximalStackDepth < 0:
    #            break
    #        e = e.older()
    #        if e == None or e.name() == None:
    #            break
    #        if e.name() in (ns + 'QThreadPrivate::start', '_ZN14QThreadPrivate5startEPv@4'):
    #            try:
    #                thrptr = e.read_var('thr').dereference()
    #                d_ptr = thrptr['d_ptr']['d'].cast(objectPrivateType).dereference()
    #                try:
    #                    objectName = d_ptr['objectName']
    #                except: # Qt 5
    #                    p = d_ptr['extraData']
    #                    if not self.isNull(p):
    #                        objectName = p.dereference()['objectName']
    #                if not objectName is None:
    #                    (data, size, alloc) = self.stringData(objectName)
    #                    if size > 0:
    #                         s = self.readMemory(data, 2 * size)
    #
    #                thread = gdb.selected_thread()
    #                inner = '{valueencoded="uf16:2:0",id="'
    #                inner += str(thread.num) + '",value="'
    #                inner += s
    #                #inner += self.encodeString(objectName)
    #                inner += '"},'
    #
    #                out += inner
    #            except:
    #                pass
    #    return out

    def threadnames(self, maximalStackDepth):
        # FIXME: This needs a proper implementation for MinGW, and only there.
        # Linux, Mac and QNX mirror the objectName() to the underlying threads,
        # so we get the names already as part of the -thread-info output.
        return '[]'
        #out = '['
        #oldthread = gdb.selected_thread()
        #if oldthread:
        #    try:
        #        objectPrivateType = gdb.lookup_type(ns + 'QObjectPrivate').pointer()
        #        inferior = self.selectedInferior()
        #        for thread in inferior.threads():
        #            thread.switch()
        #            out += self.threadname(maximalStackDepth, objectPrivateType)
        #    except:
        #        pass
        #    oldthread.switch()
        #return out + ']'


    def importPlainDumper(self, printer):
        name = printer.name.replace('::', '__')
        self.qqDumpers[name] = PlainDumper(printer)
        self.qqFormats[name] = ''

    def importPlainDumpers(self):
        for obj in gdb.objfiles():
            for printers in obj.pretty_printers + gdb.pretty_printers:
                for printer in printers.subprinters:
                    self.importPlainDumper(printer)

    def qtNamespace(self):
        self.preping('qtNamespace')
        res = self.qtNamespaceX()
        self.ping('qtNamespace')
        return res

    def findSymbol(self, symbolName):
        try:
            return toInteger(gdb.parse_and_eval("(size_t)&'%s'" % symbolName))
        except:
            return 0

    def qtNamespaceX(self):
        if not self.currentQtNamespaceGuess is None:
            return self.currentQtNamespaceGuess

        for objfile in gdb.objfiles():
            name = objfile.filename
            if name.find('/libQt5Core') >= 0:
                ns = ''

                # This only works when called from a valid frame.
                try:
                    cand = 'QArrayData::shared_null'
                    symbol = gdb.lookup_symbol(cand)[0]
                    if symbol:
                        ns = symbol.name[:-len(cand)]
                except:
                    symbol = None

                if symbol is None:
                    try:
                        # Some GDB 7.11.1 on Arch Linux.
                        cand = 'QArrayData::shared_null[0]'
                        val = gdb.parse_and_eval(cand)
                        if val.type is not None:
                            typeobj = val.type.unqualified()
                            ns = typeobj.name[:-len('QArrayData')]
                    except:
                        pass

                lenns = len(ns)
                strns = ('%d%s' % (lenns - 2, ns[:lenns - 2])) if lenns else ''

                if lenns:
                    sym = '_ZN%s7QObject11customEventEPNS_6QEventE' % strns
                else:
                    sym = '_ZN7QObject11customEventEP6QEvent'
                self.qtCustomEventFunc = self.findSymbol(sym)

                if lenns:
                    sym = '_ZN%s7QObject11customEventEPNS_6QEventE@plt' % strns
                else:
                    sym = '_ZN7QObject11customEventEP6QEvent@plt'
                self.qtCustomEventPltFunc = self.findSymbol(sym)

                sym = '_ZNK%s7QObject8propertyEPKc' % strns
                self.qtPropertyFunc = self.findSymbol(sym)

                # This might be wrong, but we can't do better: We found
                # a libQt5Core and could not extract a namespace.
                # The best guess is that there isn't any.
                self.qtNamespaceToReport = ns
                self.qtNamespace = lambda: ns
                return ns

        self.currentQtNamespaceGuess = ''
        return ''

    def assignValue(self, args):
        typeName = self.hexdecode(args['type'])
        expr = self.hexdecode(args['expr'])
        value = self.hexdecode(args['value'])
        simpleType = int(args['simpleType'])
        ns = self.qtNamespace()
        if typeName.startswith(ns):
            typeName = typeName[len(ns):]
        typeName = typeName.replace('::', '__')
        pos = typeName.find('<')
        if pos != -1:
            typeName = typeName[0:pos]
        if typeName in self.qqEditable and not simpleType:
            #self.qqEditable[typeName](self, expr, value)
            expr = gdb.parse_and_eval(expr)
            self.qqEditable[typeName](self, expr, value)
        else:
            cmd = 'set variable (%s)=%s' % (expr, value)
            gdb.execute(cmd)

    def nativeDynamicTypeName(self, address, baseType):
        # Needed for Gdb13393 test.
        nativeType = self.lookupNativeType(baseType.name)
        if nativeType is None:
            return None
        nativeTypePointer = nativeType.pointer()
        nativeValue = gdb.Value(address).cast(nativeTypePointer).dereference()
        val = nativeValue.cast(nativeValue.dynamic_type)
        return str(val.type)
        #try:
        #    vtbl = gdb.execute('info symbol {%s*}0x%x' % (baseType.name, address), to_string = True)
        #except:
        #    return None
        #pos1 = vtbl.find('vtable ')
        #if pos1 == -1:
        #    return None
        #pos1 += 11
        #pos2 = vtbl.find(' +', pos1)
        #if pos2 == -1:
        #    return None
        #return vtbl[pos1 : pos2]

    def nativeDynamicType(self, address, baseType):
        # Needed for Gdb13393 test.
        nativeType = self.lookupNativeType(baseType.name)
        if nativeType is None:
            return baseType
        nativeTypePointer = nativeType.pointer()
        nativeValue = gdb.Value(address).cast(nativeTypePointer).dereference()
        return self.fromNativeType(nativeValue.dynamic_type)

    def enumExpression(self, enumType, enumValue):
        return self.qtNamespace() + 'Qt::' + enumValue

    def lookupNativeType(self, typeName):
        nativeType = self.lookupNativeTypeHelper(typeName)
        if not nativeType is None:
            self.check(isinstance(nativeType, gdb.Type))
        return nativeType

    def lookupNativeTypeHelper(self, typeName):
        typeobj = self.typeCache.get(typeName)
        #warn('LOOKUP 1: %s -> %s' % (typeName, typeobj))
        if not typeobj is None:
            return typeobj

        if typeName == 'void':
            typeobj = gdb.lookup_type(typeName)
            self.typeCache[typeName] = typeobj
            self.typesToReport[typeName] = typeobj
            return typeobj

        #try:
        #    typeobj = gdb.parse_and_eval('{%s}&main' % typeName).typeobj
        #    if not typeobj is None:
        #        self.typeCache[typeName] = typeobj
        #        self.typesToReport[typeName] = typeobj
        #        return typeobj
        #except:
        #    pass

        # See http://sourceware.org/bugzilla/show_bug.cgi?id=13269
        # gcc produces '{anonymous}', gdb '(anonymous namespace)'
        # '<unnamed>' has been seen too. The only thing gdb
        # understands when reading things back is '(anonymous namespace)'
        if typeName.find('{anonymous}') != -1:
            ts = typeName
            ts = ts.replace('{anonymous}', '(anonymous namespace)')
            typeobj = self.lookupNativeType(ts)
            if not typeobj is None:
                self.typeCache[typeName] = typeobj
                self.typesToReport[typeName] = typeobj
                return typeobj

        #warn(" RESULT FOR 7.2: '%s': %s" % (typeName, typeobj))

        # This part should only trigger for
        # gdb 7.1 for types with namespace separators.
        # And anonymous namespaces.

        ts = typeName
        while True:
            if ts.startswith('class '):
                ts = ts[6:]
            elif ts.startswith('struct '):
                ts = ts[7:]
            elif ts.startswith('const '):
                ts = ts[6:]
            elif ts.startswith('volatile '):
                ts = ts[9:]
            elif ts.startswith('enum '):
                ts = ts[5:]
            elif ts.endswith(' const'):
                ts = ts[:-6]
            elif ts.endswith(' volatile'):
                ts = ts[:-9]
            elif ts.endswith('*const'):
                ts = ts[:-5]
            elif ts.endswith('*volatile'):
                ts = ts[:-8]
            else:
                break

        if ts.endswith('*'):
            typeobj = self.lookupNativeType(ts[0:-1])
            if not typeobj is None:
                typeobj = typeobj.pointer()
                self.typeCache[typeName] = typeobj
                self.typesToReport[typeName] = typeobj
                return typeobj

        try:
            #warn("LOOKING UP 1 '%s'" % ts)
            typeobj = gdb.lookup_type(ts)
        except RuntimeError as error:
            #warn("LOOKING UP 2 '%s' ERROR %s" % (ts, error))
            # See http://sourceware.org/bugzilla/show_bug.cgi?id=11912
            exp = "(class '%s'*)0" % ts
            try:
                typeobj = self.parse_and_eval(exp).type.target()
                #warn("LOOKING UP 3 '%s'" % typeobj)
            except:
                # Can throw 'RuntimeError: No type named class Foo.'
                pass
        except:
            #warn("LOOKING UP '%s' FAILED" % ts)
            pass

        if not typeobj is None:
            #warn('CACHING: %s' % typeobj)
            self.typeCache[typeName] = typeobj
            self.typesToReport[typeName] = typeobj

        # This could still be None as gdb.lookup_type('char[3]') generates
        # 'RuntimeError: No type named char[3]'
        #self.typeCache[typeName] = typeobj
        #self.typesToReport[typeName] = typeobj
        return typeobj

    def doContinue(self):
        gdb.execute('continue')

    def fetchStack(self, args):
        def fromNativePath(string):
            return string.replace('\\', '/')

        extraQml = int(args.get('extraqml', '0'))
        limit = int(args['limit'])
        if limit <= 0:
           limit = 10000

        self.prepare(args)
        self.output = []

        i = 0
        if extraQml:
            frame = gdb.newest_frame()
            ns = self.qtNamespace()
            needle = self.qtNamespace() + 'QV4::ExecutionEngine'
            pat = '%sqt_v4StackTrace(((%sQV4::ExecutionEngine *)0x%x)->currentContext)'
            done = False
            while i < limit and frame and not done:
                block = None
                try:
                    block = frame.block()
                except:
                    pass
                if block is not None:
                    for symbol in block:
                        if symbol.is_variable or symbol.is_argument:
                            value = symbol.value(frame)
                            typeobj = value.type
                            if typeobj.code == gdb.TYPE_CODE_PTR:
                               dereftype = typeobj.target().unqualified()
                               if dereftype.name == needle:
                                    addr = toInteger(value)
                                    expr = pat % (ns, ns, addr)
                                    res = str(gdb.parse_and_eval(expr))
                                    pos = res.find('"stack=[')
                                    if pos != -1:
                                        res = res[pos + 8:-2]
                                        res = res.replace('\\\"', '\"')
                                        res = res.replace('func=', 'function=')
                                        self.put(res)
                                        done = True
                                        break
                frame = frame.older()
                i += 1

        frame = gdb.newest_frame()
        self.currentCallContext = None
        while i < limit and frame:
            with OutputSafer(self):
                name = frame.name()
                functionName = '??' if name is None else name
                fileName = ''
                objfile = ''
                symtab = ''
                pc = frame.pc()
                sal = frame.find_sal()
                line = -1
                if sal:
                    line = sal.line
                    symtab = sal.symtab
                    if not symtab is None:
                        objfile = fromNativePath(symtab.objfile.filename)
                        fullname = symtab.fullname()
                        if fullname is None:
                            fileName = ''
                        else:
                            fileName = fromNativePath(fullname)

                if self.nativeMixed and functionName == 'qt_qmlDebugMessageAvailable':
                    interpreterStack = self.extractInterpreterStack()
                    #print('EXTRACTED INTEPRETER STACK: %s' % interpreterStack)
                    for interpreterFrame in interpreterStack.get('frames', []):
                        function = interpreterFrame.get('function', '')
                        fileName = interpreterFrame.get('file', '')
                        language = interpreterFrame.get('language', '')
                        lineNumber = interpreterFrame.get('line', 0)
                        context = interpreterFrame.get('context', 0)

                        self.put(('frame={function="%s",file="%s",'
                                 'line="%s",language="%s",context="%s"}')
                            % (function, self.hexencode(fileName), lineNumber, language, context))

                    if False and self.isInternalInterpreterFrame(functionName):
                        frame = frame.older()
                        self.put(('frame={address="0x%x",function="%s",'
                                'file="%s",line="%s",'
                                'module="%s",language="c",usable="0"}') %
                            (pc, functionName, fileName, line, objfile))
                        i += 1
                        frame = frame.older()
                        continue

                self.put(('frame={level="%s",address="0x%x",function="%s",'
                        'file="%s",line="%s",module="%s",language="c"}') %
                    (i, pc, functionName, fileName, line, objfile))

            frame = frame.older()
            i += 1
        safePrint('frames=[' + ','.join(self.output) + ']')

    def createResolvePendingBreakpointsHookBreakpoint(self, args):
        class Resolver(gdb.Breakpoint):
            def __init__(self, dumper, args):
                self.dumper = dumper
                self.args = args
                spec = 'qt_qmlDebugConnectorOpen'
                super(Resolver, self).\
                    __init__(spec, gdb.BP_BREAKPOINT, internal=True, temporary=False)

            def stop(self):
                self.dumper.resolvePendingInterpreterBreakpoint(args)
                self.enabled = False
                return False

        self.interpreterBreakpointResolvers.append(Resolver(self, args))

    def exitGdb(self, _):
        gdb.execute('quit')

    def reportResult(self, msg, args):
        print(msg)

    def profile1(self, args):
        '''Internal profiling'''
        import tempfile
        import cProfile
        tempDir = tempfile.gettempdir() + '/bbprof'
        cProfile.run('theDumper.fetchVariables(%s)' % args, tempDir)
        import pstats
        pstats.Stats(tempDir).sort_stats('time').print_stats()

    def profile2(self, args):
        import timeit
        print(timeit.repeat('theDumper.fetchVariables(%s)' % args,
            'from __main__ import theDumper', number=10))



class CliDumper(Dumper):
    def __init__(self):
        Dumper.__init__(self)
        self.childrenPrefix = '['
        self.chidrenSuffix = '] '
        self.indent = 0
        self.isCli = True


    def put(self, line):
        if self.output.endswith('\n'):
            self.output = self.output[0:-1]
        self.output += line

    def putNumChild(self, numchild):
        pass

    def putOriginalAddress(self, address):
        pass

    def fetchVariables(self, args):
        args['fancy'] = 1
        args['passexception'] = 1
        args['autoderef'] = 1
        args['qobjectnames'] = 1
        name = args['varlist']
        self.prepare(args)
        self.output = name + ' = '
        frame = gdb.selected_frame()
        value = frame.read_var(name)
        with TopLevelItem(self, name):
            self.putItem(value)
        return self.output

# Global instance.
#if gdb.parameter('height') is None:
theDumper = Dumper()
#else:
#    import codecs
#    theDumper = CliDumper()

######################################################################
#
# ThreadNames Command
#
#######################################################################

def threadnames(arg):
    return theDumper.threadnames(int(arg))

registerCommand('threadnames', threadnames)

#######################################################################
#
# Native Mixed
#
#######################################################################

class InterpreterMessageBreakpoint(gdb.Breakpoint):
    def __init__(self):
        spec = 'qt_qmlDebugMessageAvailable'
        super(InterpreterMessageBreakpoint, self).\
            __init__(spec, gdb.BP_BREAKPOINT, internal=True)

    def stop(self):
        print('Interpreter event received.')
        return theDumper.handleInterpreterMessage()

#InterpreterMessageBreakpoint()
