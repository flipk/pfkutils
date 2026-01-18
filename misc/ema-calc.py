#!/usr/bin/env python3

import math

def ema_exact_N(steps: float, target_ratio: float) -> float:
    return (2 / (1 - math.pow(1 - target_ratio, 1 / steps))) - 1

def ema_approx_N(steps: float, target_ratio: float) -> float:
    return (-2 * steps) / math.log(1 - target_ratio)


def test_1():
    steps = 10
    input_step_size = 10.0
    output_distance_desired = 5.0
    target_ratio = output_distance_desired / input_step_size
    N_exact = ema_exact_N(steps, target_ratio)
    N_approx = ema_approx_N(steps, target_ratio)

    print(f'steps = {steps}\n'
          f'input_step_size = {input_step_size}\n'
          f'output_distance_desired = {output_distance_desired}\n'
          f'target_ratio = {target_ratio}\n'
          f'N exact = {N_exact}\n'
          f'N approx = {N_approx}')

test_1()
