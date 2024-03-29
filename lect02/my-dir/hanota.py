import time
import random


class Solution:

    def hanota(self, A, B, C) -> None:
        """汉诺塔问题

        Args:
            A (List[int]): A柱
            B (List[int]): B柱，辅助
            C (List[int]): C柱
        """
        n = len(A)
        self.move(n, A, B, C)

    def move(self, n, A, B, C):
        if n == 1:
            print(A)
            print(B)
            print(C)
            print('--------')
            C.append(A[-1])
            A.pop()
            return
        else:
            # 将A上的n-1个移动至B
            self.move(n - 1, A, C, B)
            # 将A上最后一个移动至C
            C.append(A[-1])
            A.pop()
            # 将B上的n-1个移动到C
            self.move(n - 1, B, A, C)


if __name__ == '__main__':
    A = [random.randint(1, 100) for _ in range(4)]
    B = []
    C = []
    solution = Solution()
    # print(A)
    start = int(time.time())
    solution.hanota(A, B, C)
    # print(C)
    print(int(time.time()) - start)
