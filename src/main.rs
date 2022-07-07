use scraper::{Html, Selector};

// Struct for holding information about an entry on Nyaa.si
#[derive(Debug)]
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
    println!("Enter a title:");
    let mut query = String::new();
    std::io::stdin().read_line(&mut query).expect("could not read from stdin");
    
    // Gather first page of results for user query into vector
    // If query is left empty, latest uploads will be gathered
    let results = search(&*query).await;

    let choice = user_choose(results);

    Ok(())
}

// Scrapes nyaa.si website for search results and collects them.
async fn search(query: &str) -> Vec<NyaaEntry> {
    // URL used for searching, applied filters:
    // - Anime - English-translated
    let search_url = format!("https://nyaa.si/?q={}&f=0&c=1_2", query);

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
    let mut entries: Vec<NyaaEntry> = vec![];

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
fn user_choose(entries: Vec<NyaaEntry>) -> NyaaEntry {
    for entry in entries.into_iter().enumerate() {
        println!("{}. {}", (entry.0)+1, entry.1);
    }

    todo!()
}