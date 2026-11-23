{
  description = "Native, sandboxed build recipe using a local cloned tree";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-26.05";
  };

  outputs =
    { self, nixpkgs }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];
      forAllSystems = nixpkgs.lib.genAttrs systems;
      version = if (self ? shortRev) then self.shortRev else "dev";
    in
    {
      packages = forAllSystems (
        system:
        let
          pkgs = nixpkgs.legacyPackages.${system};

          microtex = pkgs.fetchFromGitHub {
            owner = "kpeeters";
            repo = "MicroTeX";
            rev = "7944eb496dec1b7ff8af4c13e0cfee279eea30b8";
            sha256 = "0imzv2c8byqnl26ragh38vx3krwbfzglvmbgmrmqkvrw5ycvjg88";
          };
        in
        {
          default = pkgs.stdenv.mkDerivation {
            pname = "cadabra2";
            inherit version;

            src = self;

            preConfigure = ''
              mkdir -p submodules
              rm -rf submodules/microtex
              cp -r ${microtex} submodules/microtex
              chmod -R u+w submodules/microtex
            '';

            enableParallelBuilding = true;
            nativeBuildInputs = [
              pkgs.cmake
              pkgs.pkg-config
              pkgs.wrapGAppsHook3
            ];

            buildInputs = [
              pkgs.openssl
              pkgs.boost
              pkgs.gmp
              pkgs.sqlite
              pkgs.glibmm
              pkgs.gtkmm3
              pkgs.adwaita-icon-theme

              (pkgs.python3.withPackages (ps: [
                ps.sympy
                ps.gmpy2
                ps.matplotlib
                ps.pyzmq
              ]))
            ];

            cmakeFlags = [
              "-DPYTHON_SITE_PATH=lib/${pkgs.python3.libPrefix}/site-packages"
            ]
            ++ (
              if pkgs.stdenv.isDarwin then
                [
                  "-DENABLE_MATHEMATICA=OFF"
                ]
              else
                [
                ]
            );

            preFixup = ''
              gappsWrapperArgs+=(
                --prefix PATH : "$out/bin"
                --prefix PYTHONPATH : "$out/lib/${pkgs.python3.libPrefix}/site-packages"
              )
            '';

            meta = {
              description = "Field-theory motivated computer algebra system";
              homepage = "https://cadabra.science";
              license = pkgs.lib.licenses.gpl3Plus;
              mainProgram = "cadabra2";
              platforms = pkgs.lib.platforms.linux ++ pkgs.lib.platforms.darwin;
            };
          };
        }
      );

      apps = forAllSystems (system: {
        default = {
          type = "app";
          program = "${self.packages.${system}.default}/bin/cadabra2";
          meta = {
            description = "Launch cadabra2 (CLI)";
          };
        };
        gui = {
          type = "app";
          program = "${self.packages.${system}.default}/bin/cadabra2-gtk";
          meta = {
            description = "Launch cadabra2 GTK frontend";
          };
        };
      });
    };
}
