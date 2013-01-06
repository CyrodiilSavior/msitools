namespace Wixl {

    class WixBuilder: WixElementVisitor {
        WixRoot root;
        MsiDatabase db;

        public WixBuilder (WixRoot root) {
            this.root = root;
        }

        public MsiDatabase build () throws GLib.Error {
            db = new MsiDatabase ();
            root.accept (this);
            return db;
        }

        public override void visit_product (WixProduct product) throws GLib.Error {
            db.info.set_codepage (int.parse (product.Codepage));
            db.info.set_author (product.Manufacturer);

            db.table_property.add ("Manufacturer", product.Manufacturer);
            db.table_property.add ("ProductLanguage", product.Codepage);
            db.table_property.add ("ProductCode", add_braces (product.Id));
            db.table_property.add ("ProductName", product.Name);
            db.table_property.add ("ProductVersion", product.Version);
            db.table_property.add ("UpgradeCode", add_braces (product.UpgradeCode));
        }

        public override void visit_package (WixPackage package) throws GLib.Error {
            db.info.set_keywords (package.Keywords);
            db.info.set_subject (package.Description);
            db.info.set_comments (package.Comments);
        }

        public override void visit_icon (WixIcon icon) throws GLib.Error {
            db.table_icon.add (icon.Id, icon.SourceFile);
        }

        public override void visit_property (WixProperty prop) throws GLib.Error {
            db.table_property.add (prop.Id, prop.Value);
        }
    }

} // Wixl
