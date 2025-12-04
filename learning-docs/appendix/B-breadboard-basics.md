# Appendix B: Breadboard Basics

## Understanding Solderless Breadboards for Prototyping

A practical guide to using breadboards for building and testing circuits without soldering.

---

## What is a Breadboard?

A **breadboard** (also called a solderless breadboard) is a reusable platform for building temporary electronic circuits. You simply push component leads and wires into the holes - no soldering required!

**Why "breadboard"?** In the early days of electronics, people literally used wooden bread cutting boards with nails to build circuits. The name stuck even though modern breadboards are plastic.

---

## Your Mini Breadboard

You have a **mini breadboard** (also called a "mini" or "half+" size):

```
┌─────────────────────────────────────────────────────────┐
│                                                         │
│  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○   ← Row 1           │
│  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○                      │
│  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○                      │
│  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○                      │
│  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○                      │
│  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○                      │
│  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○                      │
│                                                         │
│                    [  notch  ]         ← Center notch   │
│                                                         │
│  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○                      │
│  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○                      │
│  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○                      │
│  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○                      │
│  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○                      │
│  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○                      │
│  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○   ← Row 17          │
│                                                         │
└─────────────────────────────────────────────────────────┘

Typical mini breadboard: 17 columns × 10 rows (170 tie points)
Your breadboard may have slightly different dimensions.
```

**Key features:**
- Compact size - great for small projects
- No power rails (unlike full-size breadboards)
- Center notch divides the board into two halves
- Interlocking tabs on sides to connect multiple breadboards

---

## How the Holes are Connected

This is the most important thing to understand! **The holes are connected in a specific pattern inside the breadboard.**

### The Internal Connections

```
Looking at the breadboard from above:

         Column
         1 2 3 4 5 6 7 8 9 ...
       ┌─────────────────────────
Row 1  │ ○ ○ ○ ○ ○ ○ ○ ○ ○       ←── All holes in Row 1 are
       │ ═════════════════           connected horizontally!
Row 2  │ ○ ○ ○ ○ ○ ○ ○ ○ ○       ←── Row 2 is separate
       │ ═════════════════
Row 3  │ ○ ○ ○ ○ ○ ○ ○ ○ ○       ←── Row 3 is separate
       │ ═════════════════
  ...  │        ...
       │
       │      [ notch ]          ←── CENTER GAP (no connection across)
       │
Row 8  │ ○ ○ ○ ○ ○ ○ ○ ○ ○       ←── Row 8 is separate from rows above
       │ ═════════════════
Row 9  │ ○ ○ ○ ○ ○ ○ ○ ○ ○
       │ ═════════════════
Row 10 │ ○ ○ ○ ○ ○ ○ ○ ○ ○
       └─────────────────────────

═══ represents the metal strip connecting holes in that row
```

### Key Rules

1. **Holes in the same row are connected** (horizontally)
2. **Different rows are NOT connected** to each other
3. **The center notch breaks the connection** - top half and bottom half are separate
4. **Columns are NOT connected** to each other

### Visual Example of Connections

```
These holes ARE connected (same row, same side of notch):
┌───────────────────┐
│ ●━━━●━━━●━━━●━━━● │  ← All 5 holes connected by metal strip
└───────────────────┘

These holes are NOT connected (different rows):
┌───────────────────┐
│ ●   ●   ●   ●   ● │  ← Row 1
│ ○   ○   ○   ○   ○ │  ← Row 2 (separate)
│ ○   ○   ○   ○   ○ │  ← Row 3 (separate)
└───────────────────┘

These holes are NOT connected (across the notch):
┌───────────────────┐
│ ●   ●   ●   ●   ● │  ← Top section
│                   │
│    [ notch ]      │  ← NO connection across!
│                   │
│ ○   ○   ○   ○   ○ │  ← Bottom section (separate)
└───────────────────┘
```

---

## What's Inside a Breadboard

If you could see inside your breadboard, you'd find metal clips:

```
Cross-section view (side):

    Plastic top with holes
    ═══════╤═══════╤═══════╤═══════
           │       │       │
         ┌─┴─┐   ┌─┴─┐   ┌─┴─┐
         │   │   │   │   │   │      ← Metal spring clips
         │ ╠═╣   │ ╠═╣   │ ╠═╣         grip component leads
         │   │   │   │   │   │
         └───┘   └───┘   └───┘

    Each clip connects all holes in one row

Top-down view of one row's metal strip:

    ┌─────────────────────────────────────┐
    │  ╔═╗   ╔═╗   ╔═╗   ╔═╗   ╔═╗      │
    │  ║ ║   ║ ║   ║ ║   ║ ║   ║ ║      │  ← Spring clips
    │  ║ ║   ║ ║   ║ ║   ║ ║   ║ ║      │
    │  ╚═╩═══╩═╩═══╩═╩═══╩═╩═══╩═╝      │
    │      Connected metal strip         │
    └─────────────────────────────────────┘
```

---

## Mini Breadboard vs Full-Size Breadboard

Your mini breadboard is simpler than a full-size one:

```
MINI BREADBOARD (what you have):
┌─────────────────────────────────┐
│                                 │
│  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ │  ← Component area only
│  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ │
│  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ │
│  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ │
│         [ notch ]               │
│  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ │
│  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ │
│  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ │
│  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ │
│                                 │
└─────────────────────────────────┘
No power rails - you use regular rows for power


FULL-SIZE BREADBOARD:
┌─────────────────────────────────────────┐
│  + ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ + │  ← Power rail (+)
│  - ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ - │  ← Ground rail (-)
│                                         │
│    a b c d e   f g h i j                │
│  1 ○ ○ ○ ○ ○   ○ ○ ○ ○ ○                │
│  2 ○ ○ ○ ○ ○   ○ ○ ○ ○ ○                │  ← Component area
│  3 ○ ○ ○ ○ ○   ○ ○ ○ ○ ○                │     (labeled columns)
│    ... (30 rows) ...                    │
│                                         │
│  + ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ + │  ← Power rail (+)
│  - ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ - │  ← Ground rail (-)
└─────────────────────────────────────────┘
Power rails run the full length of the board
```

**On a full-size breadboard:**
- Power rails (+ and -) run horizontally along the edges
- Power rails are connected along their entire length
- Component area has 5 holes per row on each side of the center gap

**On your mini breadboard:**
- No dedicated power rails
- Use any row for power distribution
- All holes in a row are connected (typically all 17 columns)

---

## Using Your Mini Breadboard

### Setting Up Power Distribution

Since your mini breadboard has no power rails, designate rows for power:

```
Example: Using top and bottom rows for power

┌─────────────────────────────────────────────┐
│  ● ● ● ● ● ● ● ● ● ● ● ● ● ● ● ● ●        │  ← 3.3V (all connected)
│  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○        │
│  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○        │  ← Available for
│  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○        │    components
│             [ notch ]                       │
│  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○        │
│  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○        │
│  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○        │
│  ● ● ● ● ● ● ● ● ● ● ● ● ● ● ● ● ●        │  ← GND (all connected)
└─────────────────────────────────────────────┘

● = holes used for power distribution
○ = holes available for components
```

### Connecting an LED (Example)

Here's how to connect an LED with a current-limiting resistor:

```
Circuit we want to build:

    3.3V ──┬── [Resistor 330Ω] ──┬── [LED +] ──┬── [LED -] ──┬── GND
           │                     │             │             │

On the breadboard:

Row 1 (3.3V): ●━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━●
                    │
Row 2:        ○ ○ ○ R ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○   ← Resistor leg 1
                    ┃ (resistor body above board)
Row 3:        ○ ○ ○ R━━━━━━━L ○ ○ ○ ○ ○ ○ ○ ○ ○   ← Resistor leg 2 + LED anode
                              ┃ (LED body)
Row 4:        ○ ○ ○ ○ ○ ○ ○ ○ L ○ ○ ○ ○ ○ ○ ○ ○   ← LED cathode
                              │
              ... (notch) ... │
                              │
Row 10 (GND): ●━━━━━━━━━━━━━━━●━━━━━━━━━━━━━━━━━━━━━━━━━━━●

R = Resistor legs
L = LED legs
● = Power connections
━ = Internal breadboard connections (in same row)
│ = Wire jumper connecting rows
```

### Step-by-Step LED Connection

1. **Connect power source to Row 1** (this becomes your 3.3V rail)
2. **Insert resistor:**
   - One leg in Row 1 (connects to 3.3V)
   - Other leg in Row 2
3. **Insert LED:**
   - Anode (longer leg, +) in Row 2 (connects to resistor)
   - Cathode (shorter leg, -) in Row 3
4. **Connect Row 3 to GND:**
   - Use a jumper wire from Row 3 to your GND row
5. **Connect ground source to GND row**

---

## Common Components on a Breadboard

### Resistors

```
Resistor (lies flat across rows):

Row 3:  ○ ○ ●━━━━━━━━━━● ○ ○ ○
            │ [resistor] │
            leg 1      leg 2

Both legs connect to Row 3, so they're connected together!
WRONG - this is a short circuit!

Correct way (spans two different rows):

Row 3:  ○ ○ ● ○ ○ ○ ○ ○ ○    ← Leg 1 in Row 3
            │
            │ [resistor body]
            │
Row 5:  ○ ○ ● ○ ○ ○ ○ ○ ○    ← Leg 2 in Row 5
```

### LEDs

```
LED (note polarity!):

    Anode (+)          Cathode (-)
    longer leg         shorter leg
    │                  │
    │    ┌──────┐     │
    │    │  LED │     │
    │    │  ◐   │     │     ← Flat edge on cathode side
    │    └──────┘     │
    │                  │
    ▼                  ▼
Row 3:  ○ ● ○ ○ ○ ○ ○ ○ ○
Row 4:  ○ ○ ● ○ ○ ○ ○ ○ ○

Anode connects to positive (through resistor)
Cathode connects to ground
```

### Jumper Wires

```
Jumper wires connect different rows:

Row 2:  ○ ○ ● ○ ○ ○ ○ ○ ○
            │
            │  ← jumper wire
            │
Row 7:  ○ ○ ● ○ ○ ○ ○ ○ ○

This connects Row 2 to Row 7
```

### ICs (Integrated Circuits)

```
ICs straddle the center notch:

            Pin 1 (marked with dot or notch)
              ↓
Row 3:  ○ ○ ○ ● ● ● ● ○ ○   ← Pins 1-4 on top
              │ │ │ │
            ┌─┴─┴─┴─┴─┐
            │  IC     │
            │  Chip   │     ← Center notch here
            └─┬─┬─┬─┬─┘
              │ │ │ │
Row 8:  ○ ○ ○ ● ● ● ● ○ ○   ← Pins 5-8 on bottom

The notch keeps the two rows of pins separate!
Without it, opposite pins would be connected (bad!)
```

---

## Practical Example: LED Circuit with LPC1343

Let's connect an external LED to your LPC1343 using the breadboard:

### What You Need
- Mini breadboard
- 1x LED (any color)
- 1x 330Ω resistor (or 220Ω-1kΩ)
- 3x Jumper wires

### The Circuit

```
LPC1343 GPIO pin (P3.0) ──► Resistor ──► LED ──► GND

Schematic:
                    330Ω
    P3.0 ○────────/\/\/\/──────┬──────○ GND
                               │
                             ──┴──
                              \ /  LED
                             ──┬──
                               │
                               │
```

### Breadboard Layout

```
                         From LPC1343
                         P3.0 (GPIO)
                              │
                              ▼
Row 1:  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ● ○ ○ ○ ○ ○   ← Wire from GPIO
                              │
Row 2:  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ R ○ ○ ○ ○ ○   ← Resistor leg 1
                              ┃
                         [Resistor]
                              ┃
Row 3:  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ R━L ○ ○ ○ ○   ← Resistor leg 2 + LED anode
                                ┃
                              [LED]
                                ┃
Row 4:  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ L ○ ○ ○   ← LED cathode
                                │
                    jumper wire │
                                │
                To LPC1343 GND ─┘


Wire connections to LPC1343:
1. GPIO wire: LPC1343 P3.0 → Breadboard Row 1
2. GND wire:  LPC1343 GND  → Breadboard Row 4 (same row as LED cathode)
```

### Code to Test It

```c
// Blink external LED on P3.0
#include "LPC13xx.h"

int main(void) {
    // Enable GPIO clock
    LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 6);

    // Set P3.0 as output
    LPC_GPIO3->DIR |= (1 << 0);

    while (1) {
        LPC_GPIO3->DATA |= (1 << 0);   // HIGH - LED ON
        for (volatile int i = 0; i < 500000; i++);

        LPC_GPIO3->DATA &= ~(1 << 0);  // LOW - LED OFF
        for (volatile int i = 0; i < 500000; i++);
    }
}
```

---

## Common Mistakes to Avoid

### Mistake 1: Components in the Same Row

```
WRONG - Both resistor legs in same row (short circuit!):

Row 3:  ○ ○ ●━━━━━━━●━━○ ○ ○ ○ ○ ○ ○
            └─────────┘
         These are connected = short circuit!

RIGHT - Resistor spans two rows:

Row 3:  ○ ○ ● ○ ○ ○ ○ ○ ○ ○ ○ ○ ○
            │
Row 4:  ○ ○ ● ○ ○ ○ ○ ○ ○ ○ ○ ○ ○
```

### Mistake 2: IC Legs on Same Side

```
WRONG - IC pins connected together:

Row 3:  ○ ● ● ● ● ○ ○ ○   ← All 4 pins connected!
          └─┴─┴─┘
           IC chip

RIGHT - IC straddles the notch:

Row 3:  ○ ● ● ● ● ○ ○ ○   ← Top pins (separate rows each)
          │ │ │ │
        [   IC    ]
          │ │ │ │
Row 8:  ○ ● ● ● ● ○ ○ ○   ← Bottom pins (separate from top)
```

### Mistake 3: LED Backwards

```
LEDs only work in one direction!

Anode (+) = longer leg → connects to positive
Cathode (-) = shorter leg, flat edge → connects to ground

If your LED doesn't light up, try flipping it around.
```

### Mistake 4: Forgetting the Resistor

```
WRONG - LED directly connected to power:

    3.3V ────────[LED]──────── GND

    LED will burn out or blow instantly!

RIGHT - Resistor limits current:

    3.3V ────[330Ω]────[LED]──────── GND

    Current = (3.3V - ~2V) / 330Ω ≈ 4mA (safe)
```

### Mistake 5: Expecting Connections Across the Notch

```
WRONG assumption - thinking these are connected:

Row 4:  ○ ○ ○ ● ○ ○ ○ ○    ← Top section

        [    notch    ]    ← NO CONNECTION!

Row 6:  ○ ○ ○ ● ○ ○ ○ ○    ← Bottom section (NOT connected to Row 4)

If you need to connect across the notch, use a jumper wire!
```

---

## Tips for Success

### Keep It Organized

```
Good practice:
┌─────────────────────────────────────────┐
│  + + + + + + + + + + + + + + + + +      │  ← 3.3V (consistent location)
│                                         │
│      [  Components neatly placed  ]     │
│                                         │
│  - - - - - - - - - - - - - - - - -      │  ← GND (consistent location)
└─────────────────────────────────────────┘

- Use consistent colors: Red for power, Black for ground
- Keep wires short and neat
- Leave space between components
```

### Use a Multimeter to Check Connections

```
Continuity test:
1. Set multimeter to continuity mode (🔊 symbol)
2. Touch probes to two holes
3. If it beeps, they're connected
4. If not, they're separate

Great for:
- Verifying your understanding of breadboard connections
- Debugging circuits that don't work
- Checking for accidental short circuits
```

### Expanding Your Mini Breadboard

Mini breadboards often have interlocking tabs:

```
┌──────────────┐  ┌──────────────┐
│              ├──┤              │
│  Breadboard  │  │  Breadboard  │
│      1       ├──┤      2       │
│              │  │              │
└──────────────┘  └──────────────┘
       ↑              ↑
   Interlocking tabs connect them

You can combine multiple mini breadboards for larger projects!
```

---

## Quick Reference

### Connection Rules

| Configuration | Connected? |
|--------------|------------|
| Same row, same side of notch | YES |
| Different rows | NO |
| Across the center notch | NO |
| Same column, different rows | NO |

### Typical Resistor Values for LEDs

| LED Color | Forward Voltage | Resistor (3.3V supply) |
|-----------|-----------------|------------------------|
| Red | ~2.0V | 220Ω - 330Ω |
| Yellow | ~2.1V | 220Ω - 330Ω |
| Green | ~2.2V | 220Ω - 330Ω |
| Blue | ~3.0V | 100Ω - 150Ω |
| White | ~3.0V | 100Ω - 150Ω |

### Wire Color Convention

| Color | Typical Use |
|-------|-------------|
| Red | Power (3.3V, 5V, VCC) |
| Black | Ground (GND) |
| Other colors | Signals, data |

---

## Summary

1. **Breadboards let you build circuits without soldering**
2. **Holes in the same row are connected horizontally**
3. **The center notch breaks the connection between top and bottom**
4. **Different rows are NOT connected** - use jumper wires
5. **Mini breadboards have no power rails** - use regular rows instead
6. **Always use a resistor with LEDs** to limit current
7. **Check LED polarity** - longer leg is positive (anode)

---

**Return to:** [Index](../00-index.md) | [Appendix A: Hardware Setup](A-hardware-setup-stlink-uart.md)

---

*Appendix B of the Embedded C Learning Series*
*Breadboard Basics for Electronics Prototyping*
