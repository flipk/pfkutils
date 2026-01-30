import matplotlib.pyplot as plt
import random
from collections import deque

def run_simulation():
    # SETTINGS
    # A smaller window is fine to show the effect
    WINDOW_SIZE = 100      
    # We need a lot of iterations to make the drift obvious in a short simulation
    ITERATIONS = 1_000_000 
    
    # 1. Setup
    history = deque([0.0] * WINDOW_SIZE, maxlen=WINDOW_SIZE)
    naive_sum = 0.0
    
    errors = []
    iterations = []

    # 2. Run the loop
    for i in range(ITERATIONS):
        # Generate a random value (0.0 to 1.0)
        # Using numbers like 0.1 or 0.2 explicitly can also trigger it, 
        # but random noise is realistic for data processing.
        new_val = random.random()
        
        old_val = history[0] # The value about to fall off
        history.append(new_val)
        
        # METHOD A: Naive O(1) Running Sum
        # This is where the bug lives: (Sum - Old + New)
        naive_sum = naive_sum - old_val + new_val
        
        # METHOD B: Ground Truth
        # O(N) calculation - slow but accurate
        true_sum = sum(history)
        
        # Record the drift every 1000 steps to keep the graph rendering fast
        if i % 1000 == 0:
            drift = abs(naive_sum - true_sum)
            errors.append(drift)
            iterations.append(i)

    # 3. Visualize
    plt.figure(figsize=(10, 6))
    plt.plot(iterations, errors, color='red', linewidth=1, label='Accumulated Error')
    
    plt.title(f'Floating Point Drift: Naive Sliding Window (N={WINDOW_SIZE})')
    plt.xlabel('Iterations')
    plt.ylabel('Absolute Error magnitude')
    plt.grid(True, which='both', linestyle='--', linewidth=0.5)
    plt.legend()
    
    # Add a text annotation explaining the "Silent Killer"
    max_error = max(errors)
    plt.annotate(f'Max Drift: {max_error:.2e}', 
                 xy=(ITERATIONS, errors[-1]), 
                 xytext=(ITERATIONS/2, max_error),
                 arrowprops=dict(facecolor='black', shrink=0.05))

    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    run_simulation()
