// prompt: canonically print a parsed unit header keyword and name through Stigi.Emit.Writer.
def Stigi.Print.writeDeclKeyword {writer: Stigi.Emit.Writer, Stigi.Mut} (cap: Stigi.EmitCap, buf: Array[U8](cap), kind: Stigi.Ast.DeclKind): Unit =
  kind match
    case DeclDef => writer.putSlice(cap, buf, 4, "def ", 0, 4)
    case DeclLemma => writer.putSlice(cap, buf, 10, "lemma def ", 0, 10)
    case DeclVal => writer.putSlice(cap, buf, 4, "val ", 0, 4)
    case DeclEnum => writer.putSlice(cap, buf, 5, "enum ", 0, 5)
    case DeclType => writer.putSlice(cap, buf, 5, "type ", 0, 5)
    case DeclTrait => writer.putSlice(cap, buf, 6, "trait ", 0, 6)
    case DeclGiven => writer.putSlice(cap, buf, 6, "given ", 0, 6)
    case DeclCapability => writer.putSlice(cap, buf, 11, "capability ", 0, 11)
    case DeclHandler => writer.putSlice(cap, buf, 8, "handler ", 0, 8)
    case DeclExtern => writer.putSlice(cap, buf, 15, "extern \"c\" def ", 0, 15)
    case DeclRelation => writer.putSlice(cap, buf, 9, "relation ", 0, 9)
    case DeclRule => writer.putSlice(cap, buf, 9, "rule def ", 0, 9)
    case DeclTest => writer.putSlice(cap, buf, 4, "val ", 0, 4)
    case DeclNegTest => writer.putSlice(cap, buf, 4, "val ", 0, 4)
