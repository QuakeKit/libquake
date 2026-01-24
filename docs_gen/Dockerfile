FROM --platform=linux/amd64 python:3.11-slim

# Install runtime tools
# - doxygen: for parsing C++
# - graphviz: for doxygen graphs (optional)
# - git: needed for mkdocs-material hooks
# - wget/unzip: to fetch doxybook2
RUN apt-get update && apt-get install -y \
    doxygen \
    graphviz \
    git \
    wget \
    unzip \
    && rm -rf /var/lib/apt/lists/*

# Install mkdocs and material theme
RUN pip install mkdocs-material mkdocs-awesome-pages-plugin mkdoxy

# Copy the runner script
COPY docs_gen/run_docs.sh /usr/local/bin/
RUN chmod +x /usr/local/bin/run_docs.sh

# Expose MkDocs port
EXPOSE 8000

# Set working directory to project root
WORKDIR /app

# Default command
ENTRYPOINT ["/usr/local/bin/run_docs.sh"]
CMD ["build"]

