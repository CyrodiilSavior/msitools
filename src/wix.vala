namespace Wixl {

    public abstract class WixElementVisitor: Object {
        public abstract void visit_product (WixProduct product) throws GLib.Error;
        public abstract void visit_icon (WixIcon icon) throws GLib.Error;
        public abstract void visit_package (WixPackage package) throws GLib.Error;
        public abstract void visit_property (WixProperty prop) throws GLib.Error;
        public abstract void visit_media (WixMedia media) throws GLib.Error;
        public abstract void visit_directory (WixDirectory dir) throws GLib.Error;
        public abstract void visit_component (WixComponent comp) throws GLib.Error;
        public abstract void visit_feature (WixFeature feature) throws GLib.Error;
        public abstract void visit_component_ref (WixComponentRef ref) throws GLib.Error;
        public abstract void visit_remove_folder (WixRemoveFolder rm) throws GLib.Error;
        public abstract void visit_registry_value (WixRegistryValue reg) throws GLib.Error;
        public abstract void visit_file (WixFile reg) throws GLib.Error;
        public abstract void visit_shortcut (WixShortcut shortcut) throws GLib.Error;
    }

    public abstract class WixElement: Object {
        public class string name;

        public string Id { get; set; }
        public WixElement parent;
        public List<WixElement> children;

        static construct {
            Value.register_transform_func (typeof (WixElement), typeof (string), (ValueTransform)WixElement.value_to_string);
        }

        protected class HashTable<string, Type> child_types = null; // FIXME: would be nice if vala always initialize class member to null
        class construct {
            child_types = new HashTable<string, Type> (int_hash, int_equal);
        }

        public class void add_child_types (HashTable<string, Type> table, Type[] child_types) {
            foreach (var t in child_types) {
                var name = ((WixElement) Object.new (t)).name;
                table.insert (name, t);
            }
        }

        public void add_child (WixElement e) {
            e.parent = this;
            children.append (e);
        }

        public virtual void load (Xml.Node *node) throws Wixl.Error {
            if (node->name != name)
                throw new Error.FAILED ("%s: invalid node %s".printf (name, node->name));

            for (var prop = node->properties; prop != null; prop = prop->next) {
                if (prop->type == Xml.ElementType.ATTRIBUTE_NODE)
                    set_property (prop->name, get_attribute_content (prop));
            }

            for (var child = node->children; child != null; child = child->next) {
                switch (child->type) {
                case Xml.ElementType.COMMENT_NODE:
                case Xml.ElementType.TEXT_NODE:
                    continue;
                case Xml.ElementType.ELEMENT_NODE:
                    var t = child_types.lookup (child->name);
                    if (t != 0) {
                        var elem = Object.new (t) as WixElement;
                        elem.load (child);
                        add_child (elem);
                        continue;
                    }
                    break;
                }
                debug ("unhandled child %s node %s", name, child->name);
            }
        }

        public string to_string () {
            var type = get_type ();
            var klass = (ObjectClass)type.class_ref ();
            var str = "<" + name;

            var i = 0;
            foreach (var p in klass.list_properties ()) {
                var value = Value (p.value_type);
                get_property (p.name, ref value);
                var valstr = value.holds (typeof (string)) ?
                    (string)value : value.strdup_contents ();
                str += " " + p.name + "=\"" + valstr + "\"";
                i += 1;
            }

            if (children.length () != 0) {
                str += ">\n";

                foreach (var child in children) {
                    str += child.to_string () + "\n";
                }

                return str + "</" + name + ">";
            } else
                return str + "/>";
        }

        public static void value_to_string (Value src, out Value dest) {
            WixElement e = value_get_element (src);

            dest = e.to_string ();
        }

        public static WixElement? value_get_element (Value value) {
            if (! value.holds (typeof (WixElement)))
                return null;

            return (WixElement)value.get_object ();
        }

        public virtual void accept (WixElementVisitor visitor) throws GLib.Error {
            foreach (var child in children)
                child.accept (visitor);
        }
    }

    public class WixProperty: WixElement {
        static construct {
            name = "Property";
        }

        public string Value { get; set; }

        public override void accept (WixElementVisitor visitor) throws GLib.Error {
            visitor.visit_property (this);
        }
    }

    public class WixPackage: WixElement {
        static construct {
            name = "Package";
        }

        public string Keywords { get; set; }
        public string InstallerDescription { get; set; }
        public string InstallerComments { get; set; }
        public string Manufacturer { get; set; }
        public string InstallerVersion { get; set; }
        public string Languages { get; set; }
        public string Compressed { get; set; }
        public string SummaryCodepage { get; set; }
        public string Comments { get; set; }
        public string Description { get; set; }

        public override void accept (WixElementVisitor visitor) throws GLib.Error {
            base.accept (visitor);
            visitor.visit_package (this);
        }
    }

    public class WixIcon: WixElement {
        static construct {
            name = "Icon";
        }

        public string SourceFile { get; set; }

        public override void accept (WixElementVisitor visitor) throws GLib.Error {
            visitor.visit_icon (this);
        }
    }

    public abstract class WixKeyElement: WixElement {
        public string KeyPath { get; set; }
    }

    public class WixFile: WixKeyElement {
        static construct {
            name = "File";
        }

        public string DiskId { get; set; }
        public string Source { get; set; }
        public string Name { get; set; }

        public Libmsi.Record record;

        public override void accept (WixElementVisitor visitor) throws GLib.Error {
            visitor.visit_file (this);
        }
    }

    public class WixRegistryValue: WixKeyElement {
        static construct {
            name = "RegistryValue";
        }

        public string Root { get; set; }
        public string Key { get; set; }
        public string Type { get; set; }
        public string Value { get; set; }
        public string Name { get; set; }

        public override void accept (WixElementVisitor visitor) throws GLib.Error {
            visitor.visit_registry_value (this);
        }
    }

    public class WixRemoveFolder: WixElement {
        static construct {
            name = "RemoveFolder";
        }

        public string On { get; set; }

        public override void accept (WixElementVisitor visitor) throws GLib.Error {
            visitor.visit_remove_folder (this);
        }
    }

    public class WixFeature: WixElement {
        static construct {
            name = "Feature";

            add_child_types (child_types, { typeof (WixComponentRef) });
        }

        public string Level { get; set; }

        public override void accept (WixElementVisitor visitor) throws GLib.Error {
            base.accept (visitor);
            visitor.visit_feature (this);
        }
    }

    public class WixComponentRef: WixElement {
        static construct {
            name = "ComponentRef";
        }

        public override void accept (WixElementVisitor visitor) throws GLib.Error {
            visitor.visit_component_ref (this);
        }
    }

    public class WixProduct: WixElement {
        static construct {
            name = "Product";

            add_child_types (child_types, {
                typeof (WixPackage),
                typeof (WixIcon),
                typeof (WixProperty),
                typeof (WixMedia),
                typeof (WixDirectory),
                typeof (WixFeature)
            });
        }

        public string Name { get; set; }
        public string UpgradeCode { get; set; }
        public string Language { get; set; }
        public string Codepage { get; set; }
        public string Version { get; set; }
        public string Manufacturer { get; set; }

        public WixProduct () {
        }

        public override void accept (WixElementVisitor visitor) throws GLib.Error {
            base.accept (visitor);
            visitor.visit_product (this);
        }
    }

    public class WixMedia: WixElement {
        static construct {
            name = "Media";
        }

        public string Cabinet { get; set; }
        public string EmbedCab { get; set; }
        public string DiskPrompt { get; set; }

        public Libmsi.Record record;

        public override void accept (WixElementVisitor visitor) throws GLib.Error {
            visitor.visit_media (this);
        }
    }

    public class WixComponent: WixElement {
        static construct {
            name = "Component";

            add_child_types (child_types, {
                typeof (WixRemoveFolder),
                typeof (WixRegistryValue),
                typeof (WixFile)
            });
        }

        public string Guid { get; set; }
        public WixKeyElement? key;

        public override void accept (WixElementVisitor visitor) throws GLib.Error {
            base.accept (visitor);
            visitor.visit_component (this);
        }
    }

    public class WixDirectory: WixElement {
        static construct {
            name = "Directory";

            add_child_types (child_types, {
                typeof (WixDirectory),
                typeof (WixComponent),
            });
        }

        public string Name { get; set; }

        public override void accept (WixElementVisitor visitor) throws GLib.Error {
            base.accept (visitor);
            visitor.visit_directory (this);
        }
    }

    class WixRoot: WixElement {
        static construct {
            name = "Wix";

            add_child_types (child_types, { typeof (WixProduct) });
        }

        public string xmlns { get; set; }

        public WixRoot () {
        }

        public void load_xml (Xml.Doc *doc) throws Wixl.Error {
            var root = doc->children;
            load (root);

            if (root->ns != null)
                xmlns = root->ns->href;
        }
    }

} // Wixl