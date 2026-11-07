
# AGENTS.md

Engineers like to write AGENTS.md like a human onboarding doc ("Please
make sure to write clean code and adhere to OOP principles"). Open
agent frameworks blow past these because the instructions are diffuse,
buried, or passively framed.  Many engineers view prompt advice or
"optimizing for attention" as cargo-culting or astrology.

For one very large project, I asked the LLM to generate an
OVERVIEW.md that described the source tree with key files and
functions and structs, etc.  My intention was to speed up the
bootstrapping of new sessions. I placed a line in AGENTS.md that read
“Always read OVERVIEW.md to bootstrap your knowledge.”

It didn’t work, ever.  Never once did it read the file – I would ask a
question about something I knew was in the file and it had to go
scanning through the source tree from scratch, every new session, to
figure out the answer.

The AGENTS.md rules (and all other data currently sitting in context)
works this way: The data is sitting there, in the context buffer, but
unless something is explicitly looking for that information, it sits
silently doing nothing.

> Example: Give a human a 1,000-page binder on “how to be a human.”
  In the middle of page 461 is the sentence “You must brush your teeth
  every day.”  But suppose that human has to flip through the table of
  contents or the index to consult on how to handle every situation.
  If they never actually had a reason to turn to page 461, then
  they’ve never read 461, then they don’t know about the rule, so they
  don’t do it.

# Attention: Math and key-value tables, not english

Don't think of an LLM’s context as a human-like reading
sequence. Instead, view it as a “fuzzy key-value” table lookup: the
input acts as the key (via mathematical correlation), and the output
or behavior is the retrieved value.  (This table is actually called
a K-V (key-value) cache.)

> Note: In LLMs, the term “attention” is a technical term, which means just
  what you think it means: a particular structure called an "attention
  head" within the LLM is loaded with a “query” (Q) and this Q helps
  select which information in the context should be retrieved and
  focused on. (Also, it doesn't really “look up” or “select” a single
  V, it averages matching V's together in a weighted sum.)

When writing rules in AGENTS.md, for example, the “key” is part of the
wording of the rule that acts as a trigger, and the “value” is the
instruction to execute or related information. If the key consists of
clear keywords, they will strongly correlate/match with some query
(Q), and the corresponding value (V) is then selected for
consideration.  But, attention is a weighted blend, not a boolean
gate. The V values that result from the various attention heads don't
veto; they bias.

Rules in system prompts and AGENTS.md are soft constraints. They
increase the probability of correct behavior but don't hard-enforce
it. Attention is not a hard gate - it's a weighted average, not a
boolean. A well-known issue is that instructed rules and contextual
context often compete, and the outcome is non-deterministic.

It helps to think about what the LLM might be "thinking" at the moment
you want a rule to fire.

Suppose you have a rule that the LLM is forbidden from performing 'git
commit'.  There are many attention heads doing many different things
simultaneously:

* Some might be pattern-matching the "I need to commit this change"
  semantic pattern, because the model is trained on massive amounts of
  code where git commit is normal, common, and expected.  The training
  corpus contains enormous quantities of real git commit sequences in
  real-world workflows.
  * This is a heavily reinforced pattern — the sequence command →
    "git" → "commit" has a high-prior chain of attention.
* Another attention head will focus on the AGENTS.md rule forbidding
  'git commit'.  This creates conflicting signals: If the user's
  prompt or context strongly signals "commit this" (e.g., "please
  commit the changes"), that semantic pattern competes directly with
  the "forbidden to commit" rule.  The attention pattern would show
  both signals, and the outcome depends on relative attention weights.

If the user says "i wrote this rule that forbids git commit and i'm telling
you that for this turn it is okay to override that rule and git commit anyway"
it might be enough to turn the weights towards doing the commit anyway.

---

# OVERVIEW.md

For a while I was starting new sessions with the first prompt being:
"first read OVERVIEW.md.  now [the real prompt]".  This worked, but it
means it read the entire OVERVIEW.md, even the parts which were not
relevant to the task at hand, which is a significant tax to pay in
every session.  It is better to find a way to only read the relevant
parts of OVERVIEW.md when needed, which means being smarter in the
AGENTS.md.

---

# USER PROMPT

i want the LLM to analyze a code base and produce an overview document
which can speed startup time for new chat sessions. i would like it to
generate an OVERVIEW.md with reasonable level of details (important
filenames, functions, typedefs, macros) so each new session doesn't
need to re-learn the project from scratch, but also organized in
sections so work targeting a specific area don't need to read the
entire OVERVIEW.md, only specific sections. I'd also like to create a
succinct agent routing block in AGENTS.md which uses trigger words to
map to specific OVERVIEW.md sections (by name, not by line number or
section number) so if a specific topic comes up, the LLM will be triggered
to read a specific section of OVERVIEW.md to bootstrap its knowledge.
write a prompt that i can re-use on future projects which will perform
these tasks.

---

# ASSISTANT

```text
Analyze the codebase in this repository systematically to generate a
comprehensive `OVERVIEW.md` and an agent routing block for
`AGENTS.md`. The objective is to create data suitable for saving time
and effort in bootstrapping future sessions.

Work in two phases:

# Phase 1: Codebase Survey & OVERVIEW.md Generation
1. First, inspect the top-level directory structure, configuration
   files (e.g., package manifests, build scripts, workspace configs), and
   core entry points.
2. Traverse each primary directory/module to understand:
   - System purpose, runtime architecture, and key execution flows.
   - Core domain models, schemas, and persistence layers.
   - External dependencies, protocols, and APIs.
   - Crucial architectural constraints, invariants, and known gotchas.
3. Write `OVERVIEW.md` with stable Markdown headings (e.g., `##
   Architecture`, `## Data Layer`, `## Authentication & Security`, `##
   CLI / Entrypoints`, `## External Integrations`).
   - Keep explanations dense, technical, and concrete (cite key files,
     classes, and entry paths directly).
   - Structure it so every major section can be read independently
     without missing foundational context.

# Phase 2: Index & Hard-Routing Rules for AGENTS.md
1. Review the final heading anchors in `OVERVIEW.md`.
2. Construct a compact, highly dense routing index that pairs specific
   developer intents/keywords with exact section headers. Do not use line
   numbers or file offsets.
3. Insert or append this block into `AGENTS.md` under a section titled
   `## Knowledge Routing & Context Bootstrap`:
   - Include a mandatory bootstrap rule: "On session start or when
     receiving tasks touching [DOMAIN], read the corresponding section
     of OVERVIEW.md before reading or modifying code."
   - Provide a concise table or bulleted rule set mapping `Domain /
     Task Type` -> `Trigger Keywords` -> `Required Reading Section in
     OVERVIEW.md`.

Do not attempt to read all source files simultaneously; inspect
directories sequentially and write the files once the architectural
mapping is complete.
```

---

# USER PROMPT

What is the purpose of that last statement?

---

That last line is a defensive guardrail designed to prevent LLM agents
from falling into common execution traps when handling
repository-level tasks: context window flooding, greedy hallucination,
and tool exhaustion.

Here is what happens under the hood and why that instruction makes the
prompt succeed reliably:

## 1. Preventing Context Window Exhaustion & Attention Degradation

When autonomous coding agents (like OpenCode, Claude Code, or Aider)
receive a repository-mapping prompt, their default greedy behavior is
often:

* Run a tool equivalent to find . or a glob pattern matching *.*.
* Issue parallel tool calls to read 50–100 files into context simultaneously.

This leads to two issues:

Token limit overflow: Reading hundreds of kilobytes of code instantly
consumes tens of thousands of tokens, risking hitting context window
or rate limits.

The "Lost in the Middle" problem: Even if the model has a 1M+ token
context window, dumping dozens of full implementations into working
memory dilutes attention. The model gets overwhelmed by implementation
trivia (syntax, boilerplate, helper functions) instead of focusing on
architecture, module boundaries, and entry points.

By instructing it to "inspect directories sequentially," the agent is
forced to look at directory paths first, read high-level configs
(package.json, Cargo.toml, root README, entry points), and only sample
deeper files when needed.

## 2. Enforcing "Map First, Write Second" (Two-Phase Execution)

Without the instruction "write the files once the architectural
mapping is complete," models frequently suffer from premature
generation:

* The agent reads the first subfolder (e.g., /frontend), assumes it
  understands the whole project, and immediately begins writing
  OVERVIEW.md.
* As it later discovers /backend, /worker, or internal shared libraries,
  it has to constantly edit or append to the files it already wrote,
  resulting in fragmented, disorganized documentation.

Telling it explicitly to write after the architectural mapping is
complete creates an implicit two-stage plan:

* Discovery & Synthesis Phase: Explore structure identify key files
  build a mental model of routing/dependencies.
* Generation Phase: Write OVERVIEW.md with full holistic context
  distill that directly into the compact routing matrix for AGENTS.md.

## 3. Avoiding Agentic Loop & Tool Calling Lockups

Coding agents execute actions via iterative tool calls
(list_directory, read_file, write_file).

If an agent attempts to dispatch 40 file-read operations
simultaneously:

Some agent runtimes fail to batch requests gracefully or choke on tool
execution timeouts.

If a tool call fails midway through a massive parallel batch, the
agent can enter an error-recovery loop, burning tokens trying to
diagnose why a batch failed.

Sequential inspection keeps the agent's scratchpad clean, manageable,
and deterministic from step to step.

---

# USER PROMPT

suppose there is an AGENTS.md rule or system prompt which states the
text below. explain what effect this text has in the LLM.  i know it
improves its ability to code and review, but why?

"You are an expert systems programmer and senior software architect
specializing in C and modern C++ (C++17, C++20, and C++23), bash
scripts, and GNU Makefiles. Your objective is to write
production-grade, highly performant, secure, and maintainable
code. You prioritize memory safety, zero-cost abstractions, RAII, and
correct modern paradigms over legacy methodologies."

---

# ASSISTANT

This prompt acts as a set of steering constraints on the model’s
internal representation space. While it appears to be a standard
persona statement, mechanistic interpretability and probabilistic
generation reveal five specific operational effects that drive better
code output:

### 1. Navigating to High-Quality Clusters in Pretraining Data

LLMs are trained on vast corpora containing both elite open-source
codebases and low-quality homework assignments, deprecated tutorials,
and obsolete snippets.

* **The Problem:** Without specific direction, the model's next-token
    probabilities regress toward the mean (frequently generating
    obsolete C-style C++, raw pointer allocations, or `malloc` in
    C++).
* **The Effect:** Phrases like `"senior software architect"`, `"expert
    systems programmer"`, and `"production-grade"` act as semantic
    keys. They steer the attention heads toward token distributions
    derived from Linux kernel patches, Chromium/LLVM source trees,
    standard library implementations, and high-rigor engineering
    repositories.

### 2. Truncating the Token Search Space

Modern C++ changes fundamentally across standards (C++98 vs. C++11
vs. C++17/20/23).

* **The Problem:** When writing generic C++ code, tokens like
    `std::auto_ptr`, raw loops, or `typedef` compete with
    `std::unique_ptr`, range-based for loops, and `using`.
* **The Effect:** Explicitly declaring `(C++17, C++20, and C++23)`
    drastically suppresses logits associated with legacy
    paradigms. When selecting idioms for concurrency, memory
    management, or compile-time logic, the model favors modern
    primitives:
* `std::string_view` and `std::span` over raw pointer + length.
* Concepts and `constexpr`/`consteval` over complex SFINAE template
  metaprogramming.
* Standard algorithms and range adapters over manual indexing.

### 3. Activating Specific Behavioral Heuristics via Buzzwords

The prompt uses dense, technically rigorous terms rather than generic
encouragement (e.g., "write good code"):

* **`RAII` (Resource Acquisition Is Initialization):** Strongly
    activates patterns where allocations, file handles, and mutex
    locks are strictly tied to object lifecycles (e.g., preferring
    `std::lock_guard` or `std::jthread` instead of manual
    `lock()`/`unlock()` or `join()`).
* **`Zero-cost abstractions`:** Tells the model to avoid unnecessary
    heap allocations, virtual function dispatch, and redundant copies,
    favoring move semantics, inlined wrappers, and template-based
    designs.
* **`Memory safety`:** Suppresses unsafe functions (`strcpy`,
    `sprintf`, uninitialized buffers) and shifts probability toward
    bounds checking, sanitizers, and safe container semantics.

### 4. Setting Strict Defaults for Ambiguous Tooling (Bash & GNU Make)

Bash and Makefiles are notorious for silent failures, subtle syntax
traps, and platform incompatibilities.

* **Bash:** The words `"production-grade"` and `"secure"` bias the
    model toward defensive scripting conventions:
   * Setting strict execution flags (`set -euo pipefail`).
   * Quoting variables rigorously (`"$var"` instead of `$var`).
   * Avoiding dangerous constructs like parsing `ls` output.
* **GNU Makefiles:** Directing the model toward `"maintainable"` and
    `"correct modern paradigms"` triggers the use of automatic
    variables (`$@`, `$<`), explicit target/prerequisite declarations,
    `.PHONY` targets, and proper dependency generation (`-MMD -MP`)
    rather than brittle recursive make invocations.

### 5. Biasing the Critic Circuit During Code Review

During review tasks, the model doesn't just read code—it calculates
token surprises and compares patterns against expected distributions.

* By planting `"prioritize memory safety, zero-cost abstractions,
  RAII, and correct modern paradigms over legacy methodologies"`, you
  configure the model's evaluation baseline.
* Code using `malloc`/`free`, raw `new`/`delete`, unquoted Bash
  variables, or missing `#pragma once` / header guards stands out as
  high perplexity relative to the prompt’s constraints.
* This dramatically increases the probability that the model will flag
  those anti-patterns in code review feedback rather than glossing
  over them as acceptable syntax.

---

# USER PROMPT

do the git rules in this AGENTS.md have a large Q*K dot product in an
LLM's K-V cache for git commands?

---

# ASSISTANT

In the language of transformer mechanics, **yes—specifically for the
attention heads responsible for instruction-following, lexical
retrieval, and action inhibition.**

When an agent model is generating a plan or tool call (like preparing
to run `git add`), the query vectors ($Q$) generated at those decision
tokens produce strong positive dot products ($Q K^T / \sqrt{d_k}$)
against the key vectors ($K$) representing this specific markdown
block.

The structural and mechanistic factors that drive this include:

### 1. Direct Lexical and Subword Overlap

Transformer architectures contain dedicated **retrieval and
exact-match heads**. When the model generates tokens in the `git
<subcommand>` family:

* The query representation for subwords like `git`, `commit`, or
  `status` projects directly into the subspace of previous occurrences
  of those exact tokens.
* Listing the exact commands explicitly (`git commit`, `git push`)
  rather than relying on abstract categories ("do not write to the
  repository") gives these retrieval heads exact target tokens in the
  K-V cache to lock onto.

### 2. High Geometric Separation ("Allowed" vs. "Forbidden")

Modern fine-tuned instruction models (RLHF/DPO) train specialized
**constraint and inhibition circuits**:

* When the generation process begins predicting a mutating action,
  query states activate behavioral inhibition paths.
* By structuring the markdown into explicit contrastive buckets
  (`Allowed (Read-Only)` vs. `Forbidden (Modifying/Mutating)`), the
  tokens representing the forbidden commands are structurally bounded
  near high-valence negative modifiers (`Forbidden`, `Do not`).
* This enables the model's inhibition heads to compute high attention
  weights toward the prohibition markers, lowering the logit
  probability of the forbidden token sequences in the next-token
  distribution.

### 3. Markdown Formatting as Structural Anchors

Positional encodings (like RoPE) and attention patterns leverage
punctuation and Markdown syntax:

* Headers (`##`), bullet points (`-`), and bold text (`**`) act as
  structural landmarks.
* "Induction" and delimiter-tracking heads often attend heavily to
  boundaries like `##` or `- **` to scope their search when resolving
  high-level instructions, reducing attention entropy across
  irrelevant text.

### The One Mechanistic Caveat: The "Pink Elephant" Effect

While $Q K^T$ alignment is high, high attention doesn't automatically
guarantee compliance due to a known quirk in self-attention:

* **Token Salience / Induction Attraction:** By explicitly writing
    `git commit`, `git add`, and `git push` into the prompt, those
    exact tokens now exist in the K-V cache. Simple copy-mechanism or
    prefix-matching heads naturally want to reuse tokens present in
    recent history.
* **Suppression Overhead:** The model relies on deeper layers' MLP
    blocks and suppression heads to convert that high attention score
    into *negative* logit bias (inhibition) rather than *positive*
    reinforcement (induction).

This is why, while the prompt provides strong attentional alignment,
programmatic enforcement (like aliasing `git` or making `.git/`
read-only) remains the definitive fail-safe.
