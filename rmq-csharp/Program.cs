using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;

// RMQ interface. Implementations also provide:
//   static string Name { get; }
//   static int MaxN { get; }          // optional; defaults to int.MaxValue
//   static T Build(ulong[] data);
interface IRmq
{
    long Space();
    ulong Query(int l, int r);
}

record struct Input(ulong[] Data, int[] QueryL, int[] QueryR);

// Trivial implementation that computes each query on the fly.
class Naive : IRmq
{
    private readonly ulong[] _data;
    private Naive(ulong[] data) { _data = data; }

    public static string Name => "QuadraticQuery";
    // NOTE: Do not use this for the improved implementations!
    public static int MaxN => 10_000;

    public static Naive Build(ulong[] data) => new(data);

    public long Space() => (long)_data.Length * sizeof(ulong);

    public ulong Query(int l, int r)
    {
        ulong min = _data[l];
        for (int i = l + 1; i <= r; i++)
            if (_data[i] < min) min = _data[i];
        return min;
    }
}

// -------------------------------------------------------------
// TODO: Implement IRmq for additional data structures.
// -------------------------------------------------------------

static class Program
{
    static Input ReadInput(string file)
    {
        using var sr = new StreamReader(file);
        int n = 0, q = 0;
        // First line: n q
        {
            var parts = sr.ReadLine()!.Split(' ');
            n = int.Parse(parts[0]);
            q = int.Parse(parts[1]);
        }
        var data = new ulong[n];
        for (int i = 0; i < n; i++)
            data[i] = ulong.Parse(sr.ReadLine()!.Trim());
        var queryL = new int[q];
        var queryR = new int[q];
        for (int i = 0; i < q; i++)
        {
            var parts = sr.ReadLine()!.Split(' ');
            queryL[i] = int.Parse(parts[0]);
            queryR[i] = int.Parse(parts[1]);
        }
        return new Input(data, queryL, queryR);
    }

    // Bench a pre-built RMQ instance and print results in CSV format.
    static void Bench(in Input input, string name, int maxN, IRmq rmq)
    {
        Console.Error.Write($"{input.Data.Length,10}\t{name,20}\t");
        if (input.Data.Length > maxN) { Console.Error.WriteLine("skipped"); return; }

        Console.Error.Write($"{rmq.Space(),10}\t");

        int q = input.QueryL.Length;
        var sw = Stopwatch.StartNew();
        ulong sum = 0;
        for (int i = 0; i < q; i++)
            sum += rmq.Query(input.QueryL[i], input.QueryR[i]);
        sw.Stop();

        double nsPerQuery = (double)sw.Elapsed.Ticks * 100.0 / q;
        Console.WriteLine($"{input.Data.Length},{q},{name},{rmq.Space()},{sum},{nsPerQuery:F6}");
        Console.Error.WriteLine($"{sum % 1000,3}\t{nsPerQuery:F2}ns/q");
    }

    // Bench helpers — one per implementation, to pass static Name/MaxN alongside the instance.
    static void BenchNaive(in Input input) =>
        Bench(input, Naive.Name, Naive.MaxN, Naive.Build(input.Data));

    // TODO: Add bench helpers for other implementations here.

    static void Main(string[] args)
    {
        if (args.Length < 1) { Console.Error.WriteLine("Usage: rmq-csharp <input_dir>"); return; }

        Console.WriteLine("n,q,name,space,sum,time");

        string fileOrDir = args[0];
        Console.Error.WriteLine($"Reading input from \"{fileOrDir}\" ..");

        var inputs = new List<Input>();
        if (File.Exists(fileOrDir))
        {
            inputs.Add(ReadInput(fileOrDir));
        }
        else
        {
            foreach (var f in Directory.GetFiles(fileOrDir, "*.in"))
                inputs.Add(ReadInput(f));
            inputs.Sort((a, b) => a.Data.Length.CompareTo(b.Data.Length));
        }

        foreach (var input in inputs)
        {
            BenchNaive(input);
            // TODO: Add other implementations here.
        }
    }
}
