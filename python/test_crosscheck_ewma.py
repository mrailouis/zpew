import math
import unittest

import crosscheck_ewma


class EwmaReferenceTest(unittest.TestCase):
    def test_matches_the_hand_calculated_sequence(self) -> None:
        forecasts = crosscheck_ewma.ewma_forecasts([0.1, 0.3, 0.2, 0.0, -0.4], 0.5, 2)
        for actual, expected in zip(forecasts, [0.05, 0.045, 0.0225]):
            self.assertTrue(math.isclose(actual, expected, rel_tol=1e-15))

    def test_rejects_invalid_parameters(self) -> None:
        for lam in (0.0, 1.0):
            with self.subTest(lam=lam), self.assertRaises(ValueError):
                crosscheck_ewma.ewma_forecasts([0.1, 0.2], lam, 1)
        with self.assertRaises(ValueError):
            crosscheck_ewma.ewma_forecasts([0.1], 0.94, 2)

    def test_log_returns_use_the_ratio_of_closes(self) -> None:
        self.assertEqual(crosscheck_ewma.log_returns([100.0, 125.0, 62.5]),
                         [math.log(1.25), math.log(0.5)])

    def test_relative_difference_is_symmetric_and_safe_at_zero(self) -> None:
        self.assertEqual(crosscheck_ewma.relative_difference(0.0, 0.0), 0.0)
        self.assertEqual(crosscheck_ewma.relative_difference(1.0, 2.0),
                         crosscheck_ewma.relative_difference(2.0, 1.0))


if __name__ == "__main__":
    unittest.main()
