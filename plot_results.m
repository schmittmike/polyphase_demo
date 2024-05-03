
cores = 1:12;
john_t1 = [276.02 69.96 33.52 19.59 12.79 9.23 9.85 7.45 6.04 5.67 4.99 4.48];
john_t2 = [270.3 73.21 34.08 19.6 12.72 9.24 9.93 7.63 6.39 5.7 4.99 4.68];
john_avg = (john_t1 + john_t2)/2;
john_speedup = john_avg(1) ./ john_t2;
john_parallel = john_avg(1) ./ (john_avg .* cores);


figure(1);
hold off;
grid on;
title("Average Runtime and Parallel Efficiency vs. Core Count")
xlim([1 12]);
yyaxis left
plot(cores, john_avg, 'linewidth', 3)
ylabel("average runtime (s)")
xlabel("# cores")

hold on;
yyaxis right
%plot(cores, john_speedup)
plot(cores, john_parallel, 'linewidth', 3)
ylabel("parallel efficiency")

figure(2);
title("Average Runtime and Speedup vs. Core Count");
grid on;
xlim([1 12]);
hold off;
yyaxis left
plot(cores, john_avg, 'linewidth', 3)
ylabel("average runtime (s)")
xlabel("# cores")

hold on;
yyaxis right
%plot(cores, john_speedup)
plot(cores, john_speedup, 'linewidth', 3)
ylabel("speedup")

figure(3);
title("Speedup and Parallel Efficiency vs. Core Count");
grid on;
xlim([1 12]);
hold off;
yyaxis left
plot(cores, john_speedup, 'linewidth', 3)
ylabel("speedup")
hold on;
yyaxis right
plot(cores, john_parallel, 'linewidth', 3)
ylabel("parallel efficiency")
xlabel("# cores")
