[![Tests](https://github.com/dkorolev/jsopa/actions/workflows/tests.yml/badge.svg?branch=main)](https://github.com/dkorolev/jsopa/actions/workflows/tests.yml)

# `JSOPA`

The `jsopa` repository (to be moved to `C5T/jsopa`) is the OPA IR playground with JavaScript.

## Vision

### Short Term

* To convert OPA's IR into a DSL that is easy to process further,
* To put together a hacky JavaScript implementation as a transpilation example, and
* To introduce an extensive test suite, to make it easier to develop various transpilers and engines.

At the end of the short-term goals, a JavaScript-based, or C-based, transpiled policy engine could be auto-generated per a `.rego` source.

### Long Term

* **Compact in-memory data representation.**

* * Enable ~100GB of data available in `data.*` from Rego natively.

* * Dynamic real-time data updates.

* * Schema-ful type hints (i.e. `uint8`-s for picklists, or bitmasks for small sets).

* * Customize at metadata level which data "indexes" to maintain.

* * MVCC for lock-free concurrent data updates and policy applications.

* **Zero-copy policy application.**

* * Embedded / clent library use cases.

* * No JSON serialization/deserialization.

* * Native gRPC / protobuf integration.

* **Native external data sources integration.**

* * Rego policies to natively access `data.redis.*`.

* **Static type analysis of the policies.**

* * Rego is not a typed language, but IR + [gRPC] requests schema open many doors.

My view is that natively integrating OPA/Rego policies with actor models and custom in-memory data layouts, in addition to advanced static type checking and optimizations based on it, are the holy grail that OPA needs in order to power low-latency high-QPS use cases.

Besides, [C5T/Current](https://github.com/c5t/current), my high-performance C++ project is well suited to complement the long term vision above. I am currently working on such a use case in a private company, and would love to see how we can best leverage OPA.

## Contents and Usage

### Container

The main way to use this repo is via a Docker container, currently `dimakorolev/jsopa`.

(I plan to move it from DockerHub to GHCR, and push automatically via a GitHub action. — D.K.)

The container itself mimics the `opa` binary. In fact, for most commands, it transparently invokes the very binary:

```
docker run dimakorolev/jsopa version
Version: 0.41.0
Build Commit: 0d6a109-dirty
Build Timestamp: 2022-06-02T17:45:50Z
Build Hostname: 35a936b45b0c
Go Version: go1.18.3
Platform: linux/amd64
WebAssembly: available
```

Take the [A+B](https://github.com/dkorolev/jsopa/blob/main/tests/smoke/sum/policy.rego) policy example from this repo:

```
package smoke
sum := input.a + input.b
```

The extra commands added to the `opa` binary by this container are: `rego2ir`, `rego2dsl`, and `rego2js`. All three commands require two arguments: the name of the Rego package and rule to process respectively. Don't forget the `-i` flag for the inside-Docker code to process `stdin` correctly.

Here is how the DSL representation of the above policy looks like:

```
curl -s https://raw.githubusercontent.com/dkorolev/jsopa/main/tests/smoke/sum/policy.rego \
      | docker run -i dimakorolev/jsopa rego2dsl smoke sum
```

```
BeginOPADSL()

BeginStaticStrings()
  DeclareStaticString(0, "result")
  DeclareStaticString(1, "a")
  DeclareStaticString(2, "b")
EndStaticStrings()

BeginDeclareFunctions()
  BeginDeclareFunction(0, "g0.data.smoke.sum")
    BeginFunctionArguments(0, 2)
      FunctionArgument(0, 0)
      FunctionArgument(1, 1)
    EndFunctionArguments(0)
    FunctionReturnValue(0, 2)
  EndDeclareFunction(0, "g0.data.smoke.sum")
EndDeclareFunctions(1)

BeginFunction(0, "g0.data.smoke.sum")
  BeginBlock()
    ResetLocalStmt(Local(3))
    NotEqualStmt(OperandBool(true), OperandBool(false))
    DotStmt(OperandLocal(0), OperandStringIndex(1, "a"), Local(4))
    AssignVarStmt(OperandLocal(4), Local(5))
    DotStmt(OperandLocal(0), OperandStringIndex(2, "b"), Local(6))
    AssignVarStmt(OperandLocal(6), Local(7))
    CallStmtBegin(Func(BuiltinFunc(plus)), Local(8))
      CallStmtPassArg(0, OperandLocal(5))
      CallStmtPassArg(1, OperandLocal(7))
    CallStmtEnd(Func(BuiltinFunc(plus)), Local(8))
    AssignVarStmt(OperandLocal(8), Local(9))
    AssignVarOnceStmt(OperandLocal(9), Local(3))
  EndBlock()
  BeginBlock()
    IsDefinedStmt(Local(3))
    AssignVarOnceStmt(OperandLocal(3), Local(2))
  EndBlock()
  BeginBlock()
    ReturnLocalStmt(Local(2))
  EndBlock()
EndFunction(0, "g0.data.smoke.sum")

BeginPlan(0, "main")
  BeginBlock()
    CallStmtBegin(Func(0), Local(2))
      CallStmtPassArg(0, OperandLocal(0))
      CallStmtPassArg(1, OperandLocal(1))
    CallStmtEnd(Func(0), Local(2))
    AssignVarStmt(OperandLocal(2), Local(3))
    MakeObjectStmt(Local(4))
    ObjectInsertStmt(OperandStringIndex(0, "result"), OperandLocal(3), Local(4))
    ResultSetAddStmt(Local(4))
  EndBlock()
EndPlan(0, "main")

EndOPADSL()
```

The purpose of this DSL representation is to eliminate the cumbersome process of parsing the JSON with the IR. In fact, this DSL format is designed to be taken as an input by an old-school C preprocessor.

The JavaScript code, for example, is generated directly from this DSL, in a single preprocessor pass. For the tasks from the longer-term roadmap, such as type inference and integrating with more efficient data structures, this DSL enables leveraging C++ metaprogramming without having to deal with JSON parsing.

The raw IR representation for the above policy is a bit loo long and unreadable to include in the README, but you can find it [here](https://gist.github.com/dkorolev/99808d780b29dcbe398d8841b8f338ce), and the way to generate it is:

```
curl -s https://raw.githubusercontent.com/dkorolev/jsopa/main/tests/smoke/sum/policy.rego \
      | docker run -i dimakorolev/jsopa rego2ir smoke sum | jq .
```

And here is the [example JavaScript](https://gist.github.com/dkorolev/03cda1b005fda259d227e1388224d4f5) generated for the same Rego policy, same package, same rule; it is output by:

```
curl -s https://raw.githubusercontent.com/dkorolev/jsopa/main/tests/smoke/sum/policy.rego \
      | docker run -i dimakorolev/jsopa rego2js smoke sum
```

### Tests

The [`tests/`](https://github.com/dkorolev/jsopa/tree/main/tests) directory of this repository contains test policies for JSOPA.

Each `.rego` file from there would be included in the test suite. In the directory with each `.rego` file there should be a `tests.json` file, which contains one JSON per line: the test inputs.

The current test suite compared the results of the JavaScript-transpiled policy against the OPA-generated ones.

For the test to work, each `.rego` file should start with a `#!TEST` "shebang" line. The second and third space-separated components of this line should be the package name and the rule name respectively.

I plan to grow this set of tests dramatically, as its scope and coverage will be instrumental in developing a fully compliant high-performance specialized OPA engine moving forward. Would appreciate help here.

All tests are run by a [GitHub action](https://github.com/dkorolev/jsopa/actions/runs/2484298338).
