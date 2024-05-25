import argparse

from utils.job_gene import f_load_type_verify
from utils.remote_ctrl_utils import load_private_key
from utils.task_gene import (
    f_replace_folders,
    f_set_env,
)
from utils.work_gene import (
    f_accuracy_run,
    f_bot_beaver_run,
    f_bot_config,
    f_bot_laiyang_run,
    f_bot_poll_run,
    f_bound_run,
    f_latency_inter_run,
    f_latency_internet_run,
    f_latency_intra_config,
    f_latency_intra_run,
    f_load_clear,
    f_load_config,
    f_load_run,
    f_rate_clear,
    f_rate_config,
    f_rate_run,
)


def f_args_parse():
    parser = argparse.ArgumentParser(
        description="Run experiments based on the type and options provided."
    )
    parser.add_argument(
        "-u", "--user_name", type=str, required=True, help="Cloudlab user name."
    )
    parser.add_argument(
        "-k",
        "--ssh_key",
        type=str,
        required=True,
        help="SSH private key path for cloudlab.",
    )
    parser.add_argument(
        "-m",
        "--manifest_path",
        type=str,
        default="cloudlab/manifest.xml",
        help="Manifest file path.",
    )
    subparsers = parser.add_subparsers(dest="type", required=True, help="Options")
    subparsers.add_parser("env", help="Initialize basic environment.")
    parser_replace = subparsers.add_parser(
        "replace", help="Replace certain type of remote folders."
    )
    parser_ss_rate = subparsers.add_parser(
        "rate", help="Test maximum snapshot rate (Fig.10)."
    )
    parser_ss_accuracy = subparsers.add_parser(
        "accuracy", help="Test the accuracy of snapshots (Fig.11)."
    )
    parser_bound = subparsers.add_parser(
        "bound", help="Test the upper bound results for t1-t0 (Fig.13)."
    )
    parser_load = subparsers.add_parser(
        "load",
        help="Test whether Beaver will impair "
        "performance under different workload (Fig.14).",
    )
    parser_latency = subparsers.add_parser(
        "latency",
        help="Test the minimum latency between " "the repeated requests (Fig.12).",
    )
    parser_bot = subparsers.add_parser(
        "bot", help="Do the bot user case test (Table.3)."
    )
    parser_ss_rate.add_argument(
        "-s",
        "--scale",
        type=int,
        required=True,
        choices=[2, 4, 6, 8, 10, 12, 14, 16],
        help="Scale of the load balancers, must be specified.",
    )
    parser_ss_rate.add_argument(
        "-p",
        "--parallel",
        action="store_true",
        help="Whether to do snapshots in parallel.",
    )
    parser_ss_rate.add_argument(
        "-o",
        "--operation",
        type=str,
        choices=["config", "run", "clear"],
        required=True,
        help="Specify the operation of the experiment:"
        "config for configuration, run for execution,"
        "clear for removing all configuration.",
    )
    parser_bound.add_argument(
        "-o",
        "--operation",
        type=str,
        choices=["config", "run", "clear"],
        required=True,
        help="Specify the operation of the experiment:"
        "config for configuration, run for execution,"
        "clear for removing all configuration.",
    )
    parser_bound.add_argument(
        "-s",
        "--scale",
        type=int,
        required=True,
        choices=[2, 4, 6, 8, 10, 12, 14, 16],
        help="Scale of the load balancers, must be specified.",
    )
    parser_ss_accuracy.add_argument(
        "-s",
        "--scale",
        type=int,
        required=True,
        choices=[2, 4, 6, 8, 10, 12, 14, 16],
        help="Scale of the load balancers, must be specified.",
    )
    parser_ss_accuracy.add_argument(
        "-f", "--frequency", type=int, required=True, help="Frequency of snapshot."
    )
    parser_ss_accuracy.add_argument(
        "-o",
        "--operation",
        type=str,
        choices=["config", "run", "clear"],
        required=True,
        help="Specify the operation of the experiment:"
        "config for configuration, run for execution,"
        "clear for removing all configuration.",
    )
    parser_replace.add_argument(
        "-nt",
        "--node_type",
        type=str,
        required=True,
        help="Type of folders to replace.",
    )
    parser_load.add_argument(
        "-s",
        "--scale",
        type=int,
        required=True,
        choices=[2, 4, 6, 8, 10, 12, 14, 16],
        help="Scale of the load balancers, must be specified.",
    )
    parser_load.add_argument(
        "-lt", "--load_type", type=str, required=True, help="Type of workload."
    )
    parser_load.add_argument(
        "-ss", "--snapshot", action="store_true", help="Whether to do snapshots."
    )
    parser_load.add_argument(
        "-o",
        "--operation",
        type=str,
        choices=["config", "run", "clear"],
        required=True,
        help="Specify the operation of the experiment:"
        "config for configuration, run for execution,"
        "clear for removing all configuration.",
    )
    parser_latency.add_argument(
        "-o",
        "--operation",
        type=str,
        choices=["config", "run", "clear"],
        required=True,
        help="Specify the operation of the experiment:"
        "config for configuration, run for execution,"
        "clear for removing all configuration.",
    )
    parser_latency.add_argument(
        "-lt",
        "--latency_type",
        type=str,
        required=True,
        choices=["internet", "inter", "intra"],
        help="Type of latency to be tested.",
    )
    parser_bot.add_argument(
        "-o",
        "--operation",
        type=str,
        choices=["config", "run", "clear"],
        required=True,
        help="Specify the operation of the experiment:"
        "config for configuration, run for execution,"
        "clear for removing all configuration.",
    )
    parser_bot.add_argument(
        "-r", "--ratio", type=float, required=True, help="Ratio of bot."
    )
    parser_bot.add_argument(
        "-st",
        "--snapshot_type",
        type=str,
        choices=["beaver", "laiyang", "poll"],
        required=True,
        help="Type of snapshot.",
    )
    args = parser.parse_args()
    return args


def main():
    args = f_args_parse()
    if load_private_key(args.ssh_key) is None:
        exit(1)
    if args.type == "env":
        f_set_env(args.manifest_path, args.user_name, args.ssh_key, "install_env.sh")
    elif args.type == "replace":
        f_replace_folders(
            args.manifest_path,
            args.user_name,
            args.ssh_key,
            "./config.json",
            args.node_type,
        )
    elif args.type == "rate":
        cluster_scale = args.scale
        frequency_list1 = [
            300000,
            240000,
            200000,
            160000,
            140000,
            130000,
            100000,
            75000,
        ]
        frequency_list2 = [10000, 8000, 6000, 6000, 6000, 6000, 6000, 6000]
        frequency_index = cluster_scale // 2 - 1
        frequency = frequency_list2[frequency_index]
        if args.parallel:
            frequency = frequency_list1[frequency_index]
        if args.operation == "config":
            f_rate_config(
                args.manifest_path, args.user_name, args.ssh_key, cluster_scale, "./"
            )
        if args.operation == "run":
            f_rate_run(
                args.manifest_path,
                args.user_name,
                args.ssh_key,
                "config.json",
                args.parallel,
                frequency,
            )
        if args.operation == "clear":
            f_rate_clear(
                args.manifest_path, args.user_name, args.ssh_key, "config.json"
            )
    elif args.type == "bound":
        cluster_scale = args.scale
        if args.operation == "config":
            f_rate_config(
                args.manifest_path, args.user_name, args.ssh_key, cluster_scale, "./"
            )
        elif args.operation == "run":
            f_bound_run(args.manifest_path, args.user_name, args.ssh_key, "config.json")
        elif args.operation == "clear":
            f_rate_clear(
                args.manifest_path, args.user_name, args.ssh_key, "config.json"
            )
    elif args.type == "accuracy":
        cluster_scale = args.scale
        if args.operation == "config":
            f_rate_config(
                args.manifest_path, args.user_name, args.ssh_key, cluster_scale, "./"
            )
        if args.operation == "run":
            f_accuracy_run(
                args.manifest_path,
                args.user_name,
                args.ssh_key,
                "config.json",
                args.frequency,
            )
        if args.operation == "clear":
            f_rate_clear(
                args.manifest_path, args.user_name, args.ssh_key, "config.json"
            )
    elif args.type == "load":
        cluster_scale = args.scale
        f_load_type_verify(args.load_type)
        if args.operation == "config":
            f_load_config(
                args.manifest_path,
                args.user_name,
                args.ssh_key,
                cluster_scale,
                args.load_type,
                "./",
            )
        if args.operation == "run":
            f_load_run(
                args.manifest_path,
                args.user_name,
                args.ssh_key,
                args.load_type,
                args.snapshot,
                "config.json",
            )
        if args.operation == "clear":
            f_load_clear(
                args.manifest_path, args.user_name, args.ssh_key, "config.json"
            )
    elif args.type == "latency":
        if args.operation == "config":
            f_latency_intra_config(
                args.manifest_path, args.user_name, args.ssh_key, "./"
            )
        elif args.operation == "run":
            if args.latency_type == "intra":
                f_latency_intra_run(
                    args.manifest_path, args.user_name, args.ssh_key, "config.json"
                )
            elif args.latency_type == "internet":
                f_latency_internet_run(
                    args.manifest_path, args.user_name, args.ssh_key, "config.json"
                )
            elif args.latency_type == "inter":
                f_latency_inter_run(
                    args.manifest_path, args.user_name, args.ssh_key, "config.json"
                )
        elif args.operation == "clear":
            f_load_clear(
                args.manifest_path, args.user_name, args.ssh_key, "config.json"
            )
    elif args.type == "bot":
        if args.operation == "config":
            f_bot_config(args.manifest_path, args.user_name, args.ssh_key, "./")
        elif args.operation == "run":
            if args.ratio < 0 or args.ratio > 1:
                raise ValueError("Ratio must be between 0 and 1.")
            if args.snapshot_type == "beaver":
                f_bot_beaver_run(
                    args.manifest_path,
                    args.user_name,
                    args.ssh_key,
                    "config.json",
                    args.ratio,
                )
            elif args.snapshot_type == "poll":
                f_bot_poll_run(
                    args.manifest_path,
                    args.user_name,
                    args.ssh_key,
                    "config.json",
                    args.ratio,
                )
            elif args.snapshot_type == "laiyang":
                f_bot_laiyang_run(
                    args.manifest_path,
                    args.user_name,
                    args.ssh_key,
                    "config.json",
                    args.ratio,
                )
        elif args.operation == "clear":
            f_load_clear(
                args.manifest_path, args.user_name, args.ssh_key, "config.json"
            )


if __name__ == "__main__":
    main()
