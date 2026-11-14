# 1. Tightly Nested Lists (No Blank Lines)

Here is a paragraph immediately followed by a bulleted list with **no blank line** in between:
- Top-level item one (using `-`)
  - Second-level item A (indented 2 spaces, no blank line above)
  - Second-level item B with `inline_code()` and **bold text**
    * Third-level item 1 (switching marker to `*`)
    * Third-level item 2
      + Fourth-level item i (switching marker to `+`)
      + Fourth-level item ii
- Top-level item two with an immediate ordered sublist:
  1. First ordered sub-item
  2. Second ordered sub-item
     - Nested bullet inside ordered list
     - Another nested bullet with inline math: $\alpha + \beta = \gamma$
  3. Third ordered sub-item
- Top-level item three with GFM task lists:
  - [x] Completed task: parse tight sublists
  - [ ] Pending task: verify MathML rendering of $\int_0^\infty e^{-x^2} dx$
  - [x] ~~Deprecated task~~ (with strikethrough)

And an ordered list immediately following a colon without a blank line:
1. Step one: check `AGENTS.md` and `Makefile.am`
   - Sub-bullet under step one
   - Second sub-bullet under step one
2. Step two: run build
   1. Sub-step 2.1
   2. Sub-step 2.2

---

# 2. Dollar-Math / LaTeX Formulas (Inline & Display)

### Inline Math Variants
- **Simple algebraic & Greek:** Let $f(x) = ax^2 + bx + c$ where $a \neq 0$, and consider $\theta \in [0, 2\pi)$.
- **Subscripts, superscripts, and roots:** The Euclidean norm is $\|x\|_2 = \sqrt{x_1^2 + x_2^2 + \dots + x_n^2}$, and Euler's identity is $e^{i\pi} + 1 = 0$.
- **Fractions and binomials:** Recall that $\binom{n}{k} = \frac{n!}{k!(n-k)!}$ and $\lim_{x \to 0} \frac{\sin x}{x} = 1$.
- **Adjacent punctuation:** Does $(\nabla \times \mathbf{E} = -\frac{\partial \mathbf{B}}{\partial t})$ hold in vacuum? Yes, by Faraday's law: $\nabla \cdot \mathbf{B} = 0$, $\nabla \cdot \mathbf{E} = \frac{\rho}{\varepsilon_0}$.
- **Currency / Non-math Dollar Edge Cases:** If an item costs $10.50 and another costs $25.00, the total is $35.50 (whereas math $x = 10.50 + 25.00 = 35.50$ uses single dollars without spaces). Also check shell variables like `$HOME` or `${PATH}` inside code vs. prose.

### Single-Line Display Math
$$ \hat{f}(\xi) = \int_{-\infty}^{\infty} f(x)\, e^{-2\pi i x \xi}\, dx $$

### Multi-Line Display Math (Gaussian Integral & Series)
$$
\sum_{n=1}^{\infty} \frac{1}{n^2} = \frac{\pi^2}{6}, \qquad
\Gamma(z) = \int_0^\infty t^{z-1} e^{-t}\, dt
$$

### Multi-Line Aligned Equations (Navier–Stokes / Maxwell / Cauchy)
$$
\begin{aligned}
\nabla \cdot \mathbf{E} &= \frac{\rho}{\varepsilon_0} \\
\nabla \cdot \mathbf{B} &= 0 \\
\nabla \times \mathbf{E} &= -\frac{\partial \mathbf{B}}{\partial t} \\
\nabla \times \mathbf{B} &= \mu_0 \left( \mathbf{J} + \varepsilon_0 \frac{\partial \mathbf{E}}{\partial t} \right)
\end{aligned}
$$

### Matrices and Piecewise Cases
$$
A = \begin{pmatrix}
a_{11} & a_{12} & \cdots & a_{1n} \\
a_{21} & a_{22} & \cdots & a_{2n} \\
\vdots & \vdots & \ddots & \vdots \\
a_{m1} & a_{m2} & \cdots & a_{mn}
\end{pmatrix},
\qquad
\text{ReLU}(x) = \begin{cases}
x & \text{if } x > 0, \\
0 & \text{otherwise.}
\end{cases}
$$

---

# 3. Linkify & Non-Link Filename Edge Cases

### Should Be Linkified (Bare URLs & Explicit Links)
- Standard HTTPS URL: https://github.com/sst/opencode
- URL with path, query params, and fragment: https://example.com/api/v2/search?q=markdown+mathml&page=1&sort=desc#section-3
- URL with port and localhost: http://localhost:8080/healthz
- URL with parentheses (Wikipedia style): https://en.wikipedia.org/wiki/Resource_acquisition_is_initialization_(RAII)
- URL inside parentheses: (see https://en.cppreference.com/w/cpp/memory/unique_ptr for details)
- URL followed by trailing punctuation: Visit https://opencode.ai/docs, or check https://news.ycombinator.com.
- Explicit Markdown link: [MathML Core Specification](https://www.w3.org/TR/mathml-core/)

### Should NOT Be Linkified (Filenames, Identifiers, Versions)
- Markdown & documentation filenames: `AGENTS.md`, AGENTS.md, README.md, CONTRIBUTING.md, CHANGELOG.txt
- Source code filenames: main.cpp, parser.h, html_renderer.py, configure.ac, Makefile.in, CMakeLists.txt
- Config & data files: package.json, tsconfig.json, Cargo.toml, docker-compose.yml
- Dotted identifiers & member access: `std::filesystem::path`, `obj.member.value`, `req.headers.host`, `sys.stdout.write`
- Version numbers & IPs: v1.24.3, 2.0.1-beta.4

---

# 4. Tables with Formatting, Math, Alignment, and Links

| Feature / Test Case | Left-Aligned Notes | Centered Formula / Code | Right-Aligned Metric | Status |
| :--- | :--- | :---: | ---: | :---: |
| **Time Complexity** | Fast Fourier Transform (Cooley–Tukey) | $\mathcal{O}(N \log N)$ | `1,024 ms` | Passed |
| **Quadratic Formula** | Roots of $ax^2 + bx + c = 0$ | $x = \frac{-b \pm \sqrt{b^2 - 4ac}}{2a}$ | `0.0042 s` | Passed |
| **File Reference** | Do not linkify bare AGENTS.md or main.rs | `std::unique_ptr<T>` | `100.0%` | Verified |
| **Auto-Linkify** | Bare URL: https://example.org/test | `[link](https://example.org)` | `200 OK` | Active |
| **Escaped Pipes & Style** | Bitwise OR `a \| b` and ~~old~~ **new** | $\|x\|_1 = \sum_i \|x_i\|$ | `$42.99` | Done |

---

# 5. Blockquotes & Code Blocks (Ensure No Math/Linkify Inside Code)

> **Note on Parser Isolation:**
> Inside fenced code blocks and inline code spans, strings like `$x^2 + y^2 = z^2$` and `https://should-not-break-code-blocks.example.com` or `AGENTS.md` are typically rendered verbatim:
> - Bullet inside blockquote without blank line
>   - Nested bullet inside blockquote with math: $E = \hbar \omega$

```cpp
// C++20 sample: ensure $math$ and https://example.com in comments/strings stay intact
#include <iostream>
#include <memory>
#include <string_view>

int main() {
    constexpr std::string_view doc_file = "AGENTS.md";
    constexpr std::string_view endpoint = "https://api.example.com/v1/export?format=html";
    // Formula in comment: $f(x) = \sum_{i=0}^n a_i x^i$
    auto ptr = std::make_unique<double>(3.141592653589793);
    std::cout << doc_file << " -> " << endpoint << " (" << *ptr << ")\n";
    return 0;
}
```
