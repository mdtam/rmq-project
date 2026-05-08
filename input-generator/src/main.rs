use clap::Parser;
use rand::RngExt;
use std::io::Write;
use std::path::PathBuf;

#[derive(clap::Parser)]
struct Args {
    #[clap(short, value_delimiter = ',')]
    n: Vec<usize>,
    #[clap(short, default_value_t = 2_000_000)]
    q: usize,
    #[clap(long, default_value_t = usize::MAX)]
    max: usize,
    #[clap(long)]
    output: PathBuf,
}

impl Args {
    fn generate(&self) {
        let q = self.q;
        let mut rng = rand::rng();
        for &n in &self.n {
            let mut path = self.output.clone();
            path.push(format!("{n}.in"));
            eprintln!("Generating {}...", path.display());
            let file = std::fs::File::create(path).unwrap();
            let mut file = std::io::BufWriter::new(file);

            writeln!(file, "{n} {q}").unwrap();
            let mut a = vec![0u64; n];
            rng.fill(&mut a);
            for a in a {
                // make sure numbers fit in an i64.
                writeln!(file, "{}", a >> 1).unwrap();
            }
            let mut l = vec![0u64; q];
            let mut r = vec![0u64; q];
            rng.fill(&mut l);
            rng.fill(&mut r);
            for (l, r) in std::iter::zip(l, r) {
                let (l, r) = (l % n as u64, r % n as u64);
                let (l, r) = (l.min(r), l.max(r));
                writeln!(file, "{l} {r}").unwrap();
            }
        }
    }
}

fn main() {
    let args = Args::parse();
    args.generate();
}
