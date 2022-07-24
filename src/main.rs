mod bindings;
use clap::Parser;
use scraper::{Html, Selector};
use std::path::PathBuf;

// CLI options
#[derive(Debug, Parser)]
#[clap(about, version)]
struct Args {
    /// Search string to pass to nyaa.si
    #[clap(short='q', long, value_name = "title")]
    query: Option<String>,

    /// Video player executable, either on PATH or absolute path. mpv by default.
    #[clap(short='p', long, value_name = "video player")]
    player: Option<PathBuf>,

    /// Username of user to filter results by. None by default
    #[clap(short='u', long, value_name = "uploader")]
    user: Option<String>,

    /// Nyaa.si filter to use when searching. no-filter by default.
    #[clap(value_enum)]
    filter: Option<NyaaFilter>,
}

#[derive(clap::ValueEnum, Clone, Debug)]
enum NyaaFilter {
    NoFilter,
    NoRemakes,
    TrustedOnly,
}

impl std::fmt::Display for NyaaFilter {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let filter = match self {
            NyaaFilter::NoFilter => 0,
            NyaaFilter::NoRemakes => 1,
            NyaaFilter::TrustedOnly => 2,
        };
        f.write_fmt(format_args!("{}", filter))
            .expect("failed to write NyaaFilter value");
        Ok(())
    }
}

// Struct for holding information about an entry on Nyaa.si
#[derive(Debug, Clone)]
pub struct NyaaEntry {
    pub name: String,
    pub magnet: String,
    // TODO: Size, Date, Seeders, Leechers, Completed
}

impl std::fmt::Display for NyaaEntry {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_fmt(format_args!("{}", self.name))
            .expect("failed to output NyaaEntry");
        // TODO: Size, Date, etc
        Ok(())
    }
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    // Prepare variables
    let search_query = get_search_string();
    let player_path = get_player_path();
    let nyaa_filter = get_nyaa_filter();

    // Gather first page of results for user query into vector
    // If query is left empty, latest uploads will be gathered
    let results = search(search_query, nyaa_filter, Args::parse().user).await;

    // Entry chooser UI, returns user pick
    let choice = user_choose(results).unwrap();

    // Download chosen entry and return file path of download
    let output_path = unsafe { download_entry(choice) };

    open_video_player(output_path, player_path)
        .expect("could not play video");

    Ok(())
}

// Scrapes nyaa.si website for search results and collects them.
async fn search(query: String, filter: NyaaFilter, user: Option<String>) -> Vec<NyaaEntry> {
    // URL used for searching, default applied filters:
    // - Anime - English-translated
    let search_url;
    match user {
        Some(user) => { search_url = format!("https://nyaa.si/user/{}?f={}&c=1_0&q={}", user, filter, query); }
        None => { search_url = format!("https://nyaa.si/?q={}&f={}&c=1_2", query, filter); }
    }

    // Attempt to request html page
    let response = reqwest::get(search_url)
        .await
        .expect("failed to get response from URL");

    // 200 is HTTP status for OK
    if response.status() != 200 {
        panic!("nyaa server is not OK to be scraped")
    }

    let page = Html::parse_document(&*response
        .text()
        .await
        .expect("failed to decode response to UTF-8"));

    // Setup CSS selectors for matching titles and magnet links of entries
    let table_selector = Selector::parse("tbody").unwrap();
    let title_selector = Selector::parse(r#"a[title]:last-child"#).unwrap();
    let entry_selector = Selector::parse("tr").unwrap();
    let link_selector = Selector::parse(r#"a[href*="magnet:?"]"#).unwrap();

    // Select table of results, if no table is present,
    // it means that there are 0 results or other error
    let tbody = match page.select(&table_selector).next() {
        Some(table) => table,
        None => {
            println!("No results found.");
            std::process::exit(0);
        },
    };

    // Initialise variable storing results
    // note: 75 is the amount of results in one nyaa.si page
    let mut entries: Vec<NyaaEntry> = Vec::with_capacity(75);

    // For each result, extract title and link
    for entry in tbody.select(&entry_selector) {
        let title = entry
            .select(&title_selector)
            .nth(1)
            .expect("could not find title in entry")
            .text()
            .next()
            .expect("could not find text node in title");

        let link = entry
            .select(&link_selector)
            .next()
            .expect("could not find magnet link in entry")
            .value()
            .attr("href")
            .expect("could not find href attribute of magnet link");

        let structured = NyaaEntry {
            name: title.to_owned(),
            magnet: link.to_owned()
        };

        entries.push(structured);
    }
    entries

}

// Display list of results and let user pick one
fn user_choose(entries: Vec<NyaaEntry>) -> Result<NyaaEntry, &'static str> {
    // Initialise page as the first page of results
    let mut page = 1;

    // Get total no. results
    let total = entries.len();

    // Iterate through all the pages, with each page being 5 entries long
    const PAGE_LENGTH: usize = 5;

    'pages: while page <= total && page*PAGE_LENGTH <= total+PAGE_LENGTH {
        // Clear terminal
        print!("\x1B[2J\x1B[1;1H");
        println!("");

        let index = page*PAGE_LENGTH - PAGE_LENGTH..page*PAGE_LENGTH;
        // Calculate last element on the current page
        let last_in_page = {
            let mut tmp = 0;
            for i in 0..PAGE_LENGTH {
                match entries.get(page*PAGE_LENGTH-PAGE_LENGTH+i) {
                    Some(_) => tmp += 1,
                    None => break,
                }
            }
            tmp
        };

        // Check if index is out of bounds and change behaviour if it is
        if PAGE_LENGTH > last_in_page {
            // If not enough items to fill one page on the current page, then print up to the end of the vector
            for entry in entries.get(page*PAGE_LENGTH - PAGE_LENGTH..total)
                .unwrap()
                .iter()
                .enumerate() {
                println!("{}. {}", (entry.0)+1, entry.1);
            }
        } else {
            // Normal behaviour for when there are PAGE_LENGTH items available at current page
            for entry in entries.get(index.clone())
                .unwrap()
                .iter()
                .enumerate() {
                println!("{}. {}", (entry.0)+1, entry.1);
            }
        }

        // User input loop
        loop {
            // use for access to .flush()
            use std::io::Write;
            // Print user UI
            print!("\n(Page: {}) (1-{}) (n - next) (q - quit)\nMake a choice: ", page, PAGE_LENGTH);
            std::io::stdout().flush().expect("could not flush stdout");
            // Get user input
            let mut user_choice = String::new();
            std::io::stdin().read_line(&mut user_choice).expect("could not read from stdin");
            println!();
            // Parse user input to u8, assume 0 if NaN (invalid choice).
            let number: u8 = user_choice.trim().parse().unwrap_or(0);

            // Check if number provided is on page
            if number > 0 && number <= PAGE_LENGTH as u8 && number <= last_in_page as u8 {
                let selected: u8 = (page*PAGE_LENGTH - PAGE_LENGTH) as u8 + number - 1;
                let entry: &NyaaEntry = entries.get(selected as usize).expect("could not get requested entry");
                return Ok(entry.clone());
            }
            // q for exiting the program
            else if user_choice.chars().next().unwrap() == 'q' {
                println!("Quitting...");
                std::process::exit(0);
            }
            // n for next going to the next page
            else if user_choice.chars().next().unwrap() == 'n' {
                if page*PAGE_LENGTH == entries.len() || PAGE_LENGTH > last_in_page {
                    page = 1;
                    continue 'pages;
                }
                page += 1;
                continue 'pages;
            }
            else {
                println!("Invalid choice, try again.");
                continue;
            }
        }
    }
    Err("could not choose entry")
}

fn get_search_string() -> String {
    // Prompt user for search string if not provided as cli option
    let mut search_query = String::new();
    match Args::parse().query {
        Some(query) => { search_query = query; }
        None => {
            println!("Enter a title:");
            std::io::stdin().read_line(&mut search_query)
                .expect("could not read from stdin");
        }
    }
    // Replace spaces in string with '+'
    let search_query = search_query.replace(" ", "+");

    search_query
}

fn get_player_path() -> PathBuf {
    // Default player to mpv if not provided as cli option
    match Args::parse().player {
        Some(player) =>  PathBuf::from(player),
        None => PathBuf::from("mpv")
    }
}

fn get_nyaa_filter() -> NyaaFilter {
    // Default to no filter if not specified
    let nyaa_filter;
    match Args::parse().filter {
        Some(filter) => { nyaa_filter = filter; }
        None => { nyaa_filter = NyaaFilter::NoFilter; }
    }

    nyaa_filter
}

unsafe fn download_entry(entry: NyaaEntry) -> PathBuf {
    // Convert magnet URI to CString for use in ffi
    let link_cstring = std::ffi::CString::new(&entry.magnet[..])
        .expect("could not make cstring from magnet link");

    // Use c++ ffi to download using libtorrent
    let output_ptr = bindings::download_magnet(link_cstring.as_ptr()); // unsafe
    let output_path = std::ffi::CStr::from_ptr(output_ptr); // unsafe
    let output_path = output_path.to_str()
        .expect("failed to make str from cstr");
    let output_path = PathBuf::from(output_path);

    output_path
}

fn open_video_player(file_path: PathBuf, player_path: PathBuf) -> Result<(), &'static str> {
    // Open video player TODO: Actually check if it is a video instead of checking file ext
    if file_path.extension().expect("downloaded file has no extension") == "mkv" {
        println!("\nOpening {}...", &player_path.to_str().expect("could not get str from player_path"));
        std::process::Command::new(player_path)
            .arg(file_path)
            .spawn()
            .expect("failed to open video player");
        Ok(())
    } else {
        Err("file extension is not mkv. Aborting...")
    }
}
