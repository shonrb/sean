/* tree.rs
   Windows "tree" command implemented in rust.
*/
fn display_dir_tree(path: &str, padding: &str) {
    use std::fs;
    let entries: Vec<fs::DirEntry> = fs::read_dir(path)
        .expect("Not a directory")
        .map(|x| x.expect("Invalid entry"))
        .collect();

    for (i, entry) in entries.iter().enumerate() {
        let filetype = entry.file_type()
            .expect("Error getting file type");

        let entry_name = entry.file_name()
            .into_string()
            .expect("Error getting entry name");

        let is_last = i == entries.len() - 1;
        // Don't connect downwards if there is nothing to connect to
        let prefix  = if is_last { "└" } else { "├" };

        // Print the current leaf
        println!("{}{}───{}", padding, prefix, entry_name);

        // If the current entry is a directory, display its contents as a tree.
        if filetype.is_dir() {
            let entry_path = entry.path()
                .display()
                .to_string();
            // Add some extra padding to separate the new dir from its parent. 
            // Add a line representing the current directory if there is more 
            // to print in it after the new one.
            let mut new_padding = padding.to_owned();
            new_padding.push_str(
                if is_last { "    " } else { "│   " });

            display_dir_tree(&entry_path, &mut new_padding);
            continue;
        }
    }
}

fn main() {
    println!(".");
    display_dir_tree(".", "");
}
