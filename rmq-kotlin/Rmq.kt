@file:OptIn(ExperimentalUnsignedTypes::class)

import java.io.BufferedReader
import java.io.FileReader
import java.io.StreamTokenizer
import java.nio.file.Files
import java.nio.file.Path
import java.nio.file.Paths
import kotlin.math.min
interface Rmq {
    fun space(): Long
    fun query(l: Int, r: Int): ULong
}

data class Input(val data: ULongArray, val queryL: IntArray, val queryR: IntArray)

/** Trivial implementation that computes each query on the fly. */
class Naive private constructor(private val data: ULongArray) : Rmq {
    companion object {
        const val NAME = "QuadraticQuery"
        // NOTE: Do not use this for the improved implementations!
        const val MAX_N = 10_000
        fun build(data: ULongArray) = Naive(data)
    }

    override fun space() = data.size.toLong() * ULong.SIZE_BYTES

    override fun query(l: Int, r: Int): ULong {
        var min = data[l]
        for (i in l + 1..r) if (data[i] < min) min = data[i]
        return min
    }
}

// -------------------------------------------------------------
// TODO: Implement the Rmq interface for additional data structures.
// -------------------------------------------------------------

fun readInput(file: Path): Input {
    val st = StreamTokenizer(BufferedReader(FileReader(file.toFile()))).apply {
        ordinaryChars('0'.code, '9'.code)
        wordChars('0'.code, '9'.code)
    }
    fun nextInt(): Int { st.nextToken(); return st.sval.toInt() }
    fun nextULong(): ULong { st.nextToken(); return st.sval.toULong() }

    val n = nextInt(); val q = nextInt()
    val data = ULongArray(n) { nextULong() }
    val queryL = IntArray(q); val queryR = IntArray(q)
    for (i in 0 until q) { queryL[i] = nextInt(); queryR[i] = nextInt() }
    return Input(data, queryL, queryR)
}

fun bench(input: Input, name: String, maxN: Int, rmq: Rmq) {
    System.err.printf("%10d\t%20s\t", input.data.size, name)
    if (input.data.size > maxN) { System.err.println("skipped"); return }
    System.err.printf("%10d\t", rmq.space())

    val q = input.queryL.size
    val start = System.nanoTime()
    var sum = 0uL
    for (i in 0 until q) sum += rmq.query(input.queryL[i], input.queryR[i])
    val nsPerQuery = (System.nanoTime() - start).toDouble() / q

    println("${input.data.size},$q,$name,${rmq.space()},$sum,${"%.6f".format(nsPerQuery)}")
    System.err.printf("%3d\t%.2fns/q\n", (sum % 1000uL).toLong(), nsPerQuery)
}

fun benchNaive(input: Input) = bench(input, Naive.NAME, Naive.MAX_N, Naive.build(input.data))

// TODO: Add other bench helpers here.

fun main(args: Array<String>) {
    if (args.isEmpty()) { System.err.println("Usage: rmq-kotlin <input_dir>"); return }

    println("n,q,name,space,sum,time")

    val fileOrDir = Paths.get(args[0])
    System.err.println("Reading input from \"$fileOrDir\" ..")

    val inputs = if (Files.isRegularFile(fileOrDir)) {
        mutableListOf(readInput(fileOrDir))
    } else {
        Files.newDirectoryStream(fileOrDir, "*.in").use { dir ->
            dir.map { readInput(it) }.sortedBy { it.data.size }.toMutableList()
        }
    }

    for (input in inputs) {
        benchNaive(input)
        // TODO: Add other implementations here.
    }
}
